find_package(GTest CONFIG REQUIRED)

file(GLOB SRC_TEST CONFIGURE_DEPENDS *.cpp)

add_executable(mytest ${SRC_TEST})

target_link_libraries(mytest libclient libnode)
target_link_libraries(mytest GTest::gtest)

enable_testing()