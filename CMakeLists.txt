cmake_minimum_required(VERSION 3.14...3.22)

set(CMAKE_CXX_STANDARD 17)
if (BUILD_TESTS)
  list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif(BUILD_TESTS)
if (BUILD_CLIENT)
  list(APPEND VCPKG_MANIFEST_FEATURES "client")
endif(BUILD_CLIENT)
if (BUILD_BENCHMARK)
  list(APPEND VCPKG_MANIFEST_FEATURES "benchmark")
endif(BUILD_BENCHMARK)

project(Spkdfs LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(unofficial-brpc CONFIG REQUIRED)
find_package(glog CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
pkg_check_modules(LIBERASURECODE REQUIRED IMPORTED_TARGET GLOBAL erasurecode-1)

add_subdirectory(src/common)

if (BUILD_NODE)
  find_package(unofficial-braft CONFIG REQUIRED)
  find_package(unofficial-breakpad CONFIG REQUIRED)
  find_package(RocksDB CONFIG REQUIRED)
  add_subdirectory(src/node)
endif(BUILD_NODE)

option(BUILD_TESTS "Build the test target" OFF)
if (BUILD_TESTS)
  add_subdirectory(test/src)
  enable_testing()
endif(BUILD_TESTS)

option(BUILD_CLIENT "Build the client target" OFF)
if (BUILD_CLIENT)
  pkg_check_modules(FUSE3 REQUIRED IMPORTED_TARGET GLOBAL fuse3)
  add_subdirectory(src/client)
  enable_testing()
endif(BUILD_CLIENT)

option(BUILD_BENCHMARK "Build the benchmark target" OFF)
if (BUILD_BENCHMARK)
  find_package(benchmark CONFIG REQUIRED)
  add_subdirectory(benchmark)
  enable_testing()
endif(BUILD_BENCHMARK)