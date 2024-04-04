file(GLOB SRC_NODE CONFIGURE_DEPENDS *.cpp)
list(FILTER SRC_NODE EXCLUDE REGEX "main\\.cpp$")

add_library(libnode STATIC ${SRC_NODE})
target_link_libraries(libnode libcommon)
target_link_libraries(libnode unofficial::braft::braft-shared unofficial::breakpad::libbreakpad unofficial::breakpad::libbreakpad_client RocksDB::rocksdb)

add_executable(node main.cpp)
target_link_libraries(node libnode)