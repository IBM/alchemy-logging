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

# Find boost and gtest with local package install
find_package(Boost REQUIRED COMPONENTS locale regex)

# Add the library
set(library_name alog)
add_library(${library_name} src/logger.cpp)
target_include_directories(${library_name} PUBLIC include)
target_include_directories(${library_name} PUBLIC ${Boost_INCLUDE_DIRECTORIES})
target_include_directories(${library_name} PUBLIC ${nlohmann_json_SOURCE_DIR}/single_include)
target_link_libraries(${library_name} ${Boost_locale_LIBRARIES})

# Add the unit tests if enabled
option(BUILD_UNIT_TESTS "Build the unit tests" ON)
if (${BUILD_UNIT_TESTS})
  find_package(GTest REQUIRED)
  set(ut_binary_dir ${CMAKE_BINARY_DIR}/bin/test)
  set(ut_exe_name alog_test.ut)
  add_executable(${ut_exe_name} tests/logger_test.cpp tests/src/main.cpp)
  set_target_properties(
    ${ut_exe_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${ut_binary_dir}
  )
  target_include_directories(${ut_exe_name} PUBLIC tests/include)
  target_include_directories(${ut_exe_name} PUBLIC ${GTEST_INCLUDE_DIRS})
  target_link_libraries(${ut_exe_name} ${library_name})
  target_link_libraries(${ut_exe_name} ${GTEST_LIBRARIES})
  target_link_libraries(${ut_exe_name} ${Boost_LIBRARIES})
  target_link_options(${ut_exe_name} PUBLIC "-pthread")
  add_test(${ut_exe_name} "${ut_binary_dir}/${ut_exe_name}")
endif()
