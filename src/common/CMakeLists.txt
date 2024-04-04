find_path(DBG_MACRO_INCLUDE_DIRS "dbg.h")

# 指定 protobuf 文件
set(PROTO_FILE service.proto)
# 生成 protobuf 源文件和头文件
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})

file(GLOB SRC_COMMON CONFIGURE_DEPENDS *.cpp)

add_library(libcommon STATIC ${PROTO_SRCS} ${PROTO_HDRS} ${SRC_COMMON})

target_include_directories(libcommon PUBLIC include ${CMAKE_CURRENT_BINARY_DIR} ${DBG_MACRO_INCLUDE_DIRS} ../../include)

target_link_libraries(libcommon PUBLIC unofficial::brpc::brpc-shared gflags::gflags nlohmann_json::nlohmann_json glog::glog PkgConfig::LIBERASURECODE cryptopp::cryptopp)