cmake_minimum_required(VERSION 3.10)
project(StreamApp)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
add_executable(stream_app src/main.cpp src/stream_handler.cpp)
