file(GLOB SRC_FUSE CONFIGURE_DEPENDS *.cpp)

add_executable(fuse ${SRC_FUSE})

target_link_libraries(fuse libclient)
target_link_libraries(fuse PkgConfig::FUSE3)