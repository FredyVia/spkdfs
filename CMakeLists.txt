cmake_minimum_required(VERSION 3.14...3.22)

set(CMAKE_CXX_STANDARD 17)

project(Spkdfs LANGUAGES CXX)
# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# find package
find_package(PkgConfig REQUIRED)

find_package(unofficial-breakpad CONFIG REQUIRED)

find_package(GTest CONFIG REQUIRED)

# set(GFLAGS_USE_TARGET_NAMESPACE ON)
find_package(gflags CONFIG REQUIRED)

find_package(unofficial-braft CONFIG REQUIRED)

find_package(unofficial-brpc CONFIG REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)

find_package(protobuf CONFIG REQUIRED)

find_package(glog CONFIG REQUIRED)

find_package(RocksDB CONFIG REQUIRED)

find_package(cryptopp CONFIG REQUIRED)

pkg_check_modules(LIBERASURECODE REQUIRED IMPORTED_TARGET GLOBAL erasurecode-1)

pkg_check_modules(FUSE3 REQUIRED IMPORTED_TARGET GLOBAL fuse3)

# 指定 protobuf 文件
set(PROTO_FILE src/common/service.proto)
# 生成 protobuf 源文件和头文件
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})

find_path(DBG_MACRO_INCLUDE_DIRS "dbg.h")

# ---- Create standalone executable ----

file(GLOB src_common CONFIGURE_DEPENDS src/common/*.cpp)
file(GLOB src_node CONFIGURE_DEPENDS src/node/*.cpp)
file(GLOB src_client CONFIGURE_DEPENDS src/client/*.cpp)
file(GLOB src_tests CONFIGURE_DEPENDS test/src/*.cpp)
file(GLOB src_libfuse CONFIGURE_DEPENDS libfuse/*.cpp)

add_executable(node ${src_node} ${src_common} ${PROTO_SRCS})
add_executable(client ${src_client} ${src_common} ${PROTO_SRCS})
list(FILTER src_node EXCLUDE REGEX "src/node/main\\.cpp$")
list(FILTER src_client EXCLUDE REGEX "src/client/main\\.cpp$")
add_executable(tests  ${src_node} ${src_client} ${src_tests} ${src_common} ${PROTO_SRCS})
add_executable(fuse-spkdfs ${src_libfuse} ${src_common} ${src_client} ${PROTO_SRCS})

target_include_directories(node PUBLIC include ${CMAKE_CURRENT_BINARY_DIR} ${DBG_MACRO_INCLUDE_DIRS})
target_include_directories(client PUBLIC include ${CMAKE_CURRENT_BINARY_DIR} ${DBG_MACRO_INCLUDE_DIRS})
target_include_directories(tests PUBLIC include ${CMAKE_CURRENT_BINARY_DIR} ${DBG_MACRO_INCLUDE_DIRS})
target_include_directories(fuse-spkdfs PUBLIC include ${CMAKE_CURRENT_BINARY_DIR} ${DBG_MACRO_INCLUDE_DIRS})

target_link_libraries(node unofficial::braft::braft-shared gflags::gflags unofficial::brpc::brpc-shared nlohmann_json::nlohmann_json glog::glog unofficial::breakpad::libbreakpad unofficial::breakpad::libbreakpad_client RocksDB::rocksdb PkgConfig::LIBERASURECODE cryptopp::cryptopp)
target_link_libraries(client gflags::gflags unofficial::brpc::brpc-shared nlohmann_json::nlohmann_json PkgConfig::LIBERASURECODE cryptopp::cryptopp glog::glog)
target_link_libraries(tests PRIVATE GTest::gtest unofficial::braft::braft-shared gflags::gflags unofficial::brpc::brpc-shared nlohmann_json::nlohmann_json glog::glog unofficial::breakpad::libbreakpad unofficial::breakpad::libbreakpad_client RocksDB::rocksdb PkgConfig::LIBERASURECODE cryptopp::cryptopp)
target_link_libraries(fuse-spkdfs unofficial::brpc::brpc-shared PkgConfig::LIBERASURECODE PkgConfig::FUSE3 cryptopp::cryptopp)

enable_testing()