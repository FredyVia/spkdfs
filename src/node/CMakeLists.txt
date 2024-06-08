file(GLOB SRC_NODE CONFIGURE_DEPENDS *.cpp)

add_executable(node ${SRC_NODE})
target_link_libraries(node libcommon unofficial::braft::braft-shared unofficial::breakpad::libbreakpad unofficial::breakpad::libbreakpad_client RocksDB::rocksdb)