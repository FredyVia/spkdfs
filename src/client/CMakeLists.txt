add_library(libclient STATIC sdk.cpp)

add_executable(client main.cpp)

target_include_directories(libcommon PUBLIC include ${CMAKE_CURRENT_BINARY_DIR} ${DBG_MACRO_INCLUDE_DIRS} ../../include)

target_link_libraries(client libclient)
target_link_libraries(libclient libcommon)