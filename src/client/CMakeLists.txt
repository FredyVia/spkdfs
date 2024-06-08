add_library(libclient STATIC sdk.cpp)
file(GLOB SRC_FUSE CONFIGURE_DEPENDS fuse.cpp)

add_executable(client main.cpp)
add_executable(fuse ${SRC_FUSE})

target_include_directories(libcommon PUBLIC include ${CMAKE_CURRENT_BINARY_DIR} ${DBG_MACRO_INCLUDE_DIRS} ../../include)

target_link_libraries(libclient libcommon)
target_link_libraries(fuse PkgConfig::FUSE3 libclient)
target_link_libraries(client libclient)