# Set up the basic project skeleton
cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
set(PROJECT_NAME alog)
project(${PROJECT_NAME} CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Option to toggle shared libs
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

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

# Find boost with local package install
find_package(Boost REQUIRED COMPONENTS locale)

# Add the library
add_library(alog src/logger.cpp)
target_include_directories(alog PUBLIC include)
target_include_directories(alog PUBLIC ${Boost_INCLUDE_DIRECTORIES})
target_include_directories(alog PUBLIC ${nlohmann_json_SOURCE_DIR}/single_include)
