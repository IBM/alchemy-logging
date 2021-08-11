# This CMakeLists file simply aliases to the c++ implementation so that the repo
# can be used directly via CPM without the need for a SOURCE_SUBDIR
cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(alog_wrapper)
add_subdirectory(./src/cpp)
