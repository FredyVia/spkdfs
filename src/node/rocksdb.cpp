#include "node/rocksdb.h"

#include <glog/logging.h>
#include <rocksdb/write_batch.h>

#include <cstddef>
#include <nlohmann/json.hpp>
#include <string>

#include "common/exception.h"
#include "common/inode.h"
#include "common/utils.h"
namespace spkdfs {
  using namespace rocksdb;
  using namespace std;
  using json = nlohmann::json;

  RocksDB::RocksDB(const std::string& db_dir)
      : origin_dir(db_dir + "/origin"), backup_dir(db_dir + "/backup") {
    // restore db
    createDirectoryIfNotExist(origin_dir);
    createDirectoryIfNotExist(backup_dir);
    Status s
        = BackupEngine::Open(Env::Default(), BackupEngineOptions(backup_dir), &backup_engine_ptr);
    LOG_IF(ERROR, !s.ok()) << s.ToString();
    assert(s.ok());
    createDirectoryIfNotExist(db_dir);
    open();
  }

  void RocksDB::snapshot() {
    if (db_ptr == nullptr) throw runtime_error("db not ready");
    Status s = backup_engine_ptr->CreateNewBackup(db_ptr);
    LOG_IF(ERROR, !s.ok()) << s.ToString();
    assert(s.ok());
  }
  // braft promise this func is single thread
  void RocksDB::load_snapshot() {
    close();
    Status s = backup_engine_ptr->RestoreDBFromLatestBackup(origin_dir, origin_dir);
    LOG_IF(ERROR, !s.ok()) << s.ToString();
    assert(s.ok());
    open();
  }
  void RocksDB::open() {
    Options options;
    options.create_if_missing = true;
    Status s = DB::Open(options, origin_dir, &db_ptr);
    LOG_IF(ERROR, !s.ok()) << s.ToString();
    assert(s.ok());
  }

  void RocksDB::close() {
    if (db_ptr != nullptr) {
      LOG(WARNING) << "db_ptr not NULL";
      delete db_ptr;
      db_ptr = nullptr;
    }
  }

  RocksDB::~RocksDB() { close(); }

  inline rocksdb::Status RocksDB::put(const std::string& key, const std::string& value) {
    // private not check db_ptr == nullptr
    return db_ptr->Put(rocksdb::WriteOptions(), key, value);
  }

  inline rocksdb::Status RocksDB::get(const std::string& key, std::string& value) const {
    // private not check db_ptr == nullptr
    return db_ptr->Get(rocksdb::ReadOptions(), key, &value);
  }

  bool RocksDB::path_exists(const std::string& path) const {
    if (path == "/") return true;
    Status s;
    string value;
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    s = get(path, value);
    if (s.IsNotFound()) {
      return false;
    }
    if (!s.ok()) {
      throw runtime_error("unknown error rocksdb get()");
    }
    auto _json = json::parse(value);
    Inode tmpInode = _json.get<Inode>();
    return tmpInode.valid;
  }

  void RocksDB::try_to_rm(const Inode& inode) const {
    if (false == path_exists(inode.fullpath)) {
      throw MessageException(PATH_NOT_EXISTS_EXCEPTION, inode.fullpath);
    }
  }

  void RocksDB::try_to_add(const Inode& inode) const {
    // check parent_path
    string parent_path = inode.parent_path();
    DLOG(INFO) << "parent_path: " << parent_path;
    if (false == path_exists(parent_path)) {
      throw MessageException(PATH_NOT_EXISTS_EXCEPTION, parent_path);
    }
    if (true == path_exists(inode.fullpath)) {
      throw MessageException(PATH_EXISTS_EXCEPTION, inode.fullpath);
    }
  }

  // std::vector<Status> RocksDB::multiGet(const vector<std::string>& keys,
  //                                       std::vector<std::string>& values) {
  //   // private not check
  //   // if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db nt
  //   ready")); std::vector<Slice> keys; std::vector<PinnableSlice> values;

  //   for (const auto& key : keys) {
  //     keys.emplace_back(key);
  //   }
  //   values.resize(keys.size());
  //   statuses.resize(keys.size());
  //   return db_ptr->multiGet(rocksdb::ReadOptions(), key, &value);
  // }
  // std::vector<Inode>

  void RocksDB::ls(Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    Status s;
    string value;
    LOG(INFO) << "ls: inode.fullpath: " << inode.fullpath;
    s = get(inode.fullpath, value);
    LOG(INFO) << "ls: inode.value(): " << value;
    if (s.IsNotFound()) {
      if (inode.fullpath == "/")
        return;
      else
        throw MessageException(PATH_NOT_EXISTS_EXCEPTION, inode.fullpath);
    }
    if (!s.ok()) {
      throw runtime_error(s.ToString());
    }
    auto _json = json::parse(value);
    Inode tmpInode = _json.get<Inode>();
    if (tmpInode.valid == false) {
      throw MessageException(PATH_NOT_EXISTS_EXCEPTION, tmpInode.fullpath);
    }
    inode = tmpInode;
    // vector<Inode> res;
    // for (const auto& name : tmpInode.sub) {

    //   res.push_back()
    // }
  }

  void RocksDB::get(Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    ls(inode);
    if (inode.is_directory == true) {
      throw runtime_error("cannot get directory");
    }
  }

  void RocksDB::prepare_mkdir(Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    try_to_add(inode);
    inode.is_directory = true;
    inode.filesize = 0;
    inode.storage_type_ptr = nullptr;
    inode.sub = {};
    inode.valid = true;
    inode.building = false;
  }

  void RocksDB::prepare_put(Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    try_to_add(inode);
    inode.is_directory = false;
    inode.valid = true;
    inode.building = true;
  }

  void RocksDB::prepare_put_ok(Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    LOG(INFO) << "prepare put ok" << inode.value();
    try_to_rm(inode);
  }

  void RocksDB::prepare_rm(Inode& inode) {
    try_to_rm(inode);
    inode.valid = false;
  }

  void RocksDB::internal_rm(const Inode& inode) {
    Status s;
    string value;

    Inode parent_inode = get_parent_inode(inode);
    parent_inode.sub.erase(inode.filename());
    parent_inode.sub.erase(inode.filename() + "/");
    rocksdb::WriteBatch batch;
    batch.Put(parent_inode.fullpath, parent_inode.value());
    batch.Put(inode.fullpath, inode.value());
    s = db_ptr->Write(rocksdb::WriteOptions(), &batch);
    if (!s.ok()) {
      throw runtime_error("batch write not ok!" + s.ToString());
    }
  }

  Inode RocksDB::get_parent_inode(const Inode& inode) {
    Status s;
    string value;
    s = get(inode.parent_path(), value);
    Inode parent_inode;
    if (s.IsNotFound()) {
      // parent_path must be "/"
      assert(inode.parent_path() == "/");  // may be deleted before this operation
      parent_inode.fullpath = "/";
      parent_inode.is_directory = true;
      parent_inode.filesize = 0;
      parent_inode.storage_type_ptr = nullptr;
      parent_inode.sub = {};
      parent_inode.valid = true;
      parent_inode.building = false;
    } else if (s.ok()) {
      auto _json = json::parse(value);
      parent_inode = _json.get<Inode>();
    } else {
      throw runtime_error("parent add sub, internal get not ok" + s.ToString());
    }
    return parent_inode;
  }

  void RocksDB::internal_put_ok(const Inode& inode) {
    Status s;
    string value;
    auto parent_inode = get_parent_inode(inode);
    parent_inode.sub.insert(inode.filename());
    s = get(inode.fullpath, value);
    if (!s.ok()) {
      throw runtime_error("internal get not ok" + s.ToString());
    }
    auto _json = json::parse(value);
    Inode db_inode = _json.get<Inode>();
    db_inode.building = false;
    db_inode.sub = inode.sub;
    LOG(INFO) << "parent_inode:" << parent_inode.value();
    LOG(INFO) << "curr_inode: " << inode.value();
    LOG(INFO) << "db_inode: " << db_inode.value();
    rocksdb::WriteBatch batch;
    batch.Put(parent_inode.fullpath, parent_inode.value());
    batch.Put(db_inode.fullpath, db_inode.value());
    s = db_ptr->Write(rocksdb::WriteOptions(), &batch);
    if (!s.ok()) {
      throw runtime_error("batch write not ok" + s.ToString());
    }
  }

  void RocksDB::internal_put(const Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    LOG(INFO) << "internal put" << inode.value();
    Status s = put(inode.fullpath, inode.value());
    if (!s.ok()) {
      throw runtime_error("internal put not ok" + s.ToString());
    }
  }

  void RocksDB::internal_mkdir(const Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    Status s;
    LOG(INFO) << "internal mkdir" << inode.value();
    auto parent_inode = get_parent_inode(inode);
    parent_inode.sub.insert(inode.filename() + "/");
    LOG(INFO) << "parent_inode:" << parent_inode.value();
    LOG(INFO) << "curr_inode: " << inode.value();
    rocksdb::WriteBatch batch;
    batch.Put(parent_inode.fullpath, parent_inode.value());
    batch.Put(inode.fullpath, inode.value());
    s = db_ptr->Write(rocksdb::WriteOptions(), &batch);
    if (!s.ok()) {
      throw runtime_error("batch write not ok" + s.ToString());
    }
  };

}  // namespace spkdfs