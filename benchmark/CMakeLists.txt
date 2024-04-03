find_package(PkgConfig REQUIRED)
find_package(benchmark CONFIG REQUIRED)
pkg_check_modules(FUSE3 REQUIRED IMPORTED_TARGET GLOBAL fuse3)

file(GLOB SRC_BENCHMARK CONFIGURE_DEPENDS *.cpp)

add_executable(benchmark ${SRC_BENCHMARK})

target_link_libraries(benchmark libclient)
target_link_libraries(benchmark PkgConfig::FUSE3 benchmark::benchmark)