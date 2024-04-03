cmake_minimum_required(VERSION 3.14...3.22)

set(CMAKE_CXX_STANDARD 17)

project(Spkdfs LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TEST "Build the test target" OFF)
option(BUILD_FUSE "Build the fuse target" OFF)
option(BUILD_BENCHMARK "Build the benchmark target" OFF)
cmake_dependent_option(BUILD_NODE "Build the node target" ON "BUILD_TEST" OFF)

add_subdirectory(src/common)
add_subdirectory(src/client)

if (BUILD_NODE)
  add_subdirectory(src/node)
endif(BUILD_NODE)

if (BUILD_TEST)
  add_subdirectory(test/src)
  enable_testing()
endif(BUILD_TEST)

if (BUILD_FUSE)
  add_subdirectory(libfuse)
  enable_testing()
endif(BUILD_FUSE)

if (BUILD_BENCHMARK)
  add_subdirectory(benchmark)
  enable_testing()
endif(BUILD_BENCHMARK)