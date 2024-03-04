cmake_minimum_required(VERSION 3.14...3.22)

project(SpkdfsFuse LANGUAGES C)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# 查找FUSE库
find_package(PkgConfig)
pkg_check_modules(FUSE REQUIRED fuse3)

# 添加可执行文件
add_executable(fuse-spkdfs main.c)

# 包含FUSE头文件
target_include_directories(fuse-spkdfs PRIVATE ${FUSE_INCLUDE_DIRS})

# 链接FUSE库
target_link_libraries(fuse-spkdfs ${FUSE_LIBRARIES})