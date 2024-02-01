#include "node/sqlitedb.h"

#include <glog/logging.h>

#include <exception>
#include <filesystem>
#include <vector>

#include "common/inode.h"
#include "node/config.h"

namespace spkdfs {

  using namespace sqlite_orm;
  using namespace std;
  namespace fs = std::filesystem;

  SqliteDB::SqliteDB() {
    storage_ptr = std::make_unique<Storage>(_initStorage(FLAGS_data_dir + "/sqlite.db"));
  }

  std::vector<Inode> SqliteDB::ls(const Inode& inode) {
    // auto allInodes = storage_ptr->get_all<Inode>(where(
    //     (c(&Inode::parent_path) == inode.parent_path and c(&Inode::filename) == inode.filename)
    //     and c(&Inode::valid) == true));
    // if (allInodes.empty()) {
    //   if (inode.parent_path == "/" && inode.filename == "")
    //     ;
    //   else
    //     throw runtime_error("parrent");
    // }

    // if (allInodes)
    string parent_path = fs::path(inode.parent_path) / inode.filename;

    LOG(INFO) << "ls path: " << parent_path;
    return storage_ptr->get_all<Inode>(
        where((c(&Inode::parent_path) == parent_path) and c(&Inode::valid) == true));
  }

  void SqliteDB::prepare_mkdir(Inode& inode) {
    auto allInodes = storage_ptr->get_all<Inode>(where(
        (c(&Inode::parent_path) == inode.parent_path and c(&Inode::filename) == inode.filename)
        and c(&Inode::valid) == true));
    if (!allInodes.empty()) {
      throw runtime_error("path already exists");
    }
    inode.is_directory = true;
    inode.filesize = 0;
    inode.storage_type = StorageType::STORAGETYPE_REPLICA;
    inode.blk_ids = {};
    inode.valid = true;
    inode.building = false;
  }

  void SqliteDB::internal_mkdir(const Inode& inode) { storage_ptr->insert(inode); }

  std::vector<int> SqliteDB::get(const Inode& inode) {
    auto allInodes = storage_ptr->get_all<Inode>(where(
        (c(&Inode::parent_path) == inode.parent_path or c(&Inode::filename) == inode.filename)));
    if (!allInodes.empty()) {
      throw runtime_error("path already exists");
    }
    // for(auto)
    return {};
  }

  void SqliteDB::prepare_put(Inode& inode) {
    auto allInodes = storage_ptr->get_all<Inode>(where(
        (c(&Inode::parent_path) == inode.parent_path or c(&Inode::filename) == inode.filename)));
    if (!allInodes.empty()) {
      throw runtime_error("path already exists");
    }
    internal_put(inode);
  }

  void SqliteDB::internal_put(const Inode& inode) { storage_ptr->insert(inode); }

  auto reset_all() {
    // auto allInodes = storage_ptr->get_all<Inode>(where(
    //     (c(&Inode::parent_path) == inode.parent_path or c(&Inode::filename) == inode.filename)));
    // if (!allInodes.empty()) {
    //   throw runtime_error("path already exists");
    // }
    // return storage_ptr->insert(inode);
  }
}  // namespace spkdfs