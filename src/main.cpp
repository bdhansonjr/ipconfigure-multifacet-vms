#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <csignal>
#include <iostream>
#include <mutex>
#include <thread>

static std::atomic<bool> capture_requested(false);
static std::atomic<bool> shutting_down(false);
static std::mutex io_mutex;

static GstFlowReturn on_new_sample(GstAppSink *appsink, gpointer /*user_data*/) {
	GstSample *sample = gst_app_sink_pull_sample(appsink);
	if (!sample) {
		std::lock_guard<std::mutex> lock(io_mutex);
		std::cerr << "Failed to pull sample." << std::endl;
		return GST_FLOW_ERROR;
	}

	GstBuffer *buffer = gst_sample_get_buffer(sample);
	GstMapInfo map;

	GstCaps *caps = gst_sample_get_caps(sample);
	GstStructure *s = gst_caps_get_structure(caps, 0);
	int width = 0, height = 0;
	gst_structure_get_int(s, "width", &width);
	gst_structure_get_int(s, "height", &height);

	if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
		// Appsink caps force RGB
		cv::Mat frame(height, width, CV_8UC3, (void *)map.data);

		if (capture_requested.exchange(false)) {
			static int img_id = 0;
			char filename[64];
			snprintf(filename, sizeof(filename), "capture_%03d.png", img_id++);
			try {
				cv::imwrite(filename, frame);
				std::lock_guard<std::mutex> lock(io_mutex);
				std::cout << "Saved triggered frame: " << filename << std::endl;
			} catch (const std::exception &ex) {
				std::lock_guard<std::mutex> lock(io_mutex);
				std::cerr << "Failed to save image: " << ex.what() << std::endl;
			}
		}

		gst_buffer_unmap(buffer, &map);
	}

	gst_sample_unref(sample);
	return GST_FLOW_OK;
}

static gboolean bus_call(GstBus * /*bus*/, GstMessage *msg, gpointer data) {
	GMainLoop *loop = static_cast<GMainLoop *>(data);
	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_EOS: {
			std::lock_guard<std::mutex> lock(io_mutex);
			std::cout << "End of stream" << std::endl;
			g_main_loop_quit(loop);
			break;
		}
		case GST_MESSAGE_ERROR: {
			GError *err = nullptr;
			gchar *dbg = nullptr;
			gst_message_parse_error(msg, &err, &dbg);
			{
				std::lock_guard<std::mutex> lock(io_mutex);
				std::cerr << "Error: " << (err ? err->message : "unknown") << std::endl;
				if (dbg) std::cerr << "Debug: " << dbg << std::endl;
			}
			g_clear_error(&err);
			g_free(dbg);
			g_main_loop_quit(loop);
			break;
		}
		default:
			break;
	}
	return TRUE;
}

static void handle_sigint(int) {
	shutting_down = true;
}

int main(int argc, char *argv[]) {
	gst_init(&argc, &argv);

	// Optional args: destination host and port
	std::string host = "127.0.0.1";
	int port = 5000;
	if (argc >= 2) host = argv[1];
	if (argc >= 3) port = std::atoi(argv[2]);

	signal(SIGINT, handle_sigint);

	// Build pipeline with tee: source -> convert -> tee -> branch A (appsink), branch B (encode + rtp + udpsink)
	// Using queue elements to decouple branches
	// Note: appsink expects RGB
	char pipeline_desc[1024];
	snprintf(
		pipeline_desc,
		sizeof(pipeline_desc),
		"autovideosrc ! videoconvert ! video/x-raw,format=RGB ! tee name=t "
		"t. ! queue leaky=2 max-size-buffers=2 ! appsink name=sink emit-signals=true drop=true max-buffers=1 caps=\\\"video/x-raw,format=RGB\\\" "
		"t. ! queue ! videoconvert ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=2000 key-int-max=30 ! h264parse config-interval=1 ! rtph264pay pt=96 ! udpsink host=%s port=%d",
		host.c_str(), port);

	GError *error = nullptr;
	GstElement *pipeline = gst_parse_launch(pipeline_desc, &error);
	if (!pipeline) {
		std::cerr << "Pipeline creation failed: " << (error ? error->message : "unknown") << std::endl;
		g_clear_error(&error);
		return -1;
	}

	// Appsink callback
	GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
	GstAppSinkCallbacks callbacks = { nullptr, nullptr, on_new_sample };
	gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, nullptr, nullptr);

	// Main loop and bus
	GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
	GstBus *bus = gst_element_get_bus(pipeline);
	guint bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
	gst_object_unref(bus);

	{
		std::lock_guard<std::mutex> lock(io_mutex);
		std::cout << "Streaming to " << host << ":" << port << std::endl;
		std::cout << "Press ENTER to capture a still image. Ctrl+C to stop." << std::endl;
	}

	// Keyboard thread for capture trigger
	std::thread key_listener([]() {
		while (!shutting_down.load()) {
			int c = std::cin.get();
			if (c == '\n') {
				capture_requested = true;
			}
			if (!std::cin.good()) {
				break;
			}
		}
	});
	key_listener.detach();

	// Run loop until interrupted
	g_main_loop_run(loop);

	// Shutdown
	shutting_down = true;
	gst_element_set_state(pipeline, GST_STATE_NULL);
	g_source_remove(bus_watch_id);
	g_main_loop_unref(loop);
	gst_object_unref(sink);
	gst_object_unref(pipeline);

	return 0;
}