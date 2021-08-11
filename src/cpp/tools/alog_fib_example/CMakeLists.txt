# Set up the basic project skeleton
cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
set(PROJECT_NAME alog_fib_example)
project(${PROJECT_NAME} CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Use CPM to fetch alog
include(cmake/CPM.cmake)
CPMAddPackage(
  NAME alog
  GITHUB_REPOSITORY IBM/alchemy-logging
  GIT_TAG CppBuild-19
  GIT_SHALLOW true
  OPTIONS
    "BUILD_UNIT_TESTS OFF"
)

# Add the executable
set(exe_name alog_fib_example)
add_executable(${exe_name} main.cpp src/fibonacci.cpp src/util.cpp)
target_link_libraries(${exe_name} alog)
target_include_directories(
  ${exe_name}
  PRIVATE
  include
  ${alog_SOURCE_DIR}/include
)
target_link_options(${exe_name} PUBLIC "-pthread")
