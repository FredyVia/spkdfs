cmake_minimum_required(VERSION 3.22)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0012 NEW)
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
find_package(Threads REQUIRED)

find_package(GTest CONFIG REQUIRED)

set(GFLAGS_USE_TARGET_NAMESPACE ON)
find_package(gflags CONFIG REQUIRED)

find_package(unofficial-braft CONFIG REQUIRED)

find_package(unofficial-brpc CONFIG REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)

find_package(SqliteOrm REQUIRED)

find_package(Protobuf REQUIRED)

find_package(glog CONFIG REQUIRED)
# 指定 protobuf 文件
set(PROTO_FILE src/common/service.proto)
# 生成 protobuf 源文件和头文件
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})

# ---- Create standalone executable ----

file(GLOB src_common CONFIGURE_DEPENDS src/common/*.cpp)
file(GLOB src_node CONFIGURE_DEPENDS src/node/*.cpp)
file(GLOB src_client CONFIGURE_DEPENDS src/client/*.cpp)
file(GLOB src_tests CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/test/src/*.cpp)

add_executable(node ${src_node} ${src_common} ${PROTO_SRCS})
add_executable(client ${src_client} ${src_common} ${PROTO_SRCS})

list(FILTER src_node EXCLUDE REGEX "src/node/main\\.cpp$")
list(FILTER src_client EXCLUDE REGEX "src/client/main\\.cpp$")

add_executable(tests  ${src_node} ${src_client} ${src_tests} ${src_common} ${PROTO_SRCS})

target_include_directories(node PUBLIC ${PROJECT_SOURCE_DIR}/include ${PROTO_HDRS} ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(client PUBLIC ${PROJECT_SOURCE_DIR}/include ${PROTO_HDRS} ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(tests PUBLIC ${PROJECT_SOURCE_DIR}/include ${PROTO_HDRS} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(node unofficial::braft::braft-static gflags::gflags unofficial::brpc::brpc-static sqlite_orm::sqlite_orm nlohmann_json::nlohmann_json glog::glog)
target_link_libraries(client gflags::gflags  unofficial::brpc::brpc-static)
target_link_libraries(tests PRIVATE GTest::gtest unofficial::braft::braft-static gflags::gflags unofficial::brpc::brpc-static sqlite_orm::sqlite_orm nlohmann_json::nlohmann_json glog::glog)

enable_testing() 

