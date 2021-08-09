# Set up the basic project skeleton
cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
set(PROJECT_NAME alog)
project(${PROJECT_NAME} CXX)

# Set up CPM to fetch the json dependency and fetch it
include(cmake/CPM.cmake)
CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    VERSION 3.9.1
    GIT_SHALLOW true
    OPTIONS
      "JSON_BuildTests OFF"
)
