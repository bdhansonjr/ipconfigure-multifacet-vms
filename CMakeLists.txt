# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(StreamApp VERSION 1.0)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include header files from include/
include_directories(include)

# Add the main executable
add_executable(cpp-streaming-app
    src/main.cpp
)

# Add test executable (if Google Test is available)
enable_testing()

# Optional: Add unit tests (if present)
add_executable(runTests
    tests/test_main.cpp
)

# Link Google Test
find_package(GTest REQUIRED)
target_link_libraries(runTests GTest::gtest GTest::gtest_main pthread)

# Register test suite
add_test(NAME AllTests COMMAND runTests)
