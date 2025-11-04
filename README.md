Brian Hanson, Jr.

IPConfigure Technical Showcase

October 30, 2025

# Project Overview
This project represents the culmination of video stream handler in C++ using GStreamer integrated with REST APIs, Motion Detection using OpenCV that can be visualized in a VMS Dashboard.

# Build Instructions

Prereqs (Ubuntu):
- GStreamer 1.0 core, base, good/bad plugins, dev headers
- OpenCV dev

Install:

```
sudo apt-get update -y
sudo apt-get install -y \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad gstreamer1.0-libav libopencv-dev
```

Build:

```
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

Run sender (webcam -> tee -> appsink capture + RTP/UDP H264):

```
./cpp-streaming-app 127.0.0.1 5000
```

Press ENTER in the terminal to save a still image (`capture_###.png`). Ctrl+C to stop.

Receive with GStreamer (H264 depayload -> decode -> display):

```
gst-launch-1.0 -v udpsrc port=5000 caps="application/x-rtp, media=video, encoding-name=H264, payload=96" \
  ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false
```

Notes:
- If your webcam requires a specific format, replace `autovideosrc` with `v4l2src device=/dev/videoX` and adjust caps.
- For lower latency, keep `x264enc tune=zerolatency` and ensure frequent keyframes (`key-int-max`).
- To stream to a remote host, pass its IP as the first argument and open port on receiver.
# File Structure

```
cpp-streaming-app/
├── CMakeLists.txt         # Build config
├── README.md              # Project overview
├── .gitignore             # Ignore build files
├── .github/               # GitHub Actions CI/CD configuration
│   └── workflows/
│       └── build.yml/     # Build workflow script
├── src/                   # Source code
│   ├── main.cpp
│   └── stream_handler.cpp
├── include/               # Header files
│   └── stream_handler.h
├── tests/                 # Unit tests
│   └── test_stream.cpp
├── data/                  # Sample input (e.g., video files)
├── scripts/               # Bash/Python utilities
└── docker/                # Optional Dockerfiles
```

# Example Usage/Screenshots


# Technologies Used
- C++11
- GStreamer
- REST, SOAP, SOA

- Crow
- Google Test
- CMake
- AWS


