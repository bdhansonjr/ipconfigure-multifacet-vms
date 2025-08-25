# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(StreamApp VERSION 1.0)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Dependencies: GStreamer (core, app) and OpenCV
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

find_package(OpenCV REQUIRED)

# Includes
include_directories(
    include
    ${GST_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

link_directories(
    ${GST_LIBRARY_DIRS}
)

add_compile_options(${GST_CFLAGS_OTHER})

# Main executable
add_executable(cpp-streaming-app src/main.cpp)

target_link_libraries(cpp-streaming-app
    ${GST_LIBRARIES}
    ${OpenCV_LIBS}
)
