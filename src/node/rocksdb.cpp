#include "node/rocksdb.h"

#include <glog/logging.h>
#include <rocksdb/write_batch.h>

#include <cstddef>
#include <nlohmann/json.hpp>
#include <string>

#include "common/inode.h"
#include "common/utils.h"
namespace spkdfs {
  using namespace rocksdb;
  using namespace std;
  using json = nlohmann::json;
  void PathLocks::lock(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mapMutex);
    LOG(INFO) << "lock: " << key << endl;
    // 如果键不存在，则创建一个新的锁
    if (locks.find(key) == locks.end()) {
      locks[key] = std::make_unique<std::mutex>();
    }

    // 获取对应的互斥锁，但不在此处锁定
    auto& mutex = locks[key];

    // 释放映射互斥量，避免在持有它的情况下锁定资源锁，从而减少锁的粒度
    lock.unlock();

    // 锁定对应的资源
    mutex->lock();
  }

  void PathLocks::unlock(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mapMutex);  // 只读访问映射
    LOG(INFO) << "unlock: " << key << endl;
    auto it = locks.find(key);
    if (it != locks.end()) {
      it->second->unlock();
    }
  }

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
  rocksdb::Status RocksDB::put(const std::string& key, const std::string& value) {
    // private not check
    // if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");

    return db_ptr->Put(rocksdb::WriteOptions(), key, value);
  }
  rocksdb::Status RocksDB::get(const std::string& key, std::string& value) const {
    // private not check
    // if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");

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
      throw runtime_error("path not exists");
    }
  }

  void RocksDB::try_to_add(const Inode& inode) const {
    // check parent_path
    string parent_path = inode.parent_path();
    DLOG(INFO) << "parent_path: " << parent_path;
    if (false == path_exists(parent_path)) {
      throw runtime_error("parent path not exists");
    }
    if (true == path_exists(inode.fullpath)) {
      throw runtime_error("path already exists");
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
    if (s.IsNotFound() && inode.fullpath != "/") {
      throw runtime_error("path not exist");
    }
    if (s.ok()) {
      auto _json = json::parse(value);
      Inode tmpInode = _json.get<Inode>();
      if (tmpInode.valid == false) {
        throw runtime_error("not valid");
      }
      inode = tmpInode;
    }
    // vector<Inode> res;
    // for (const auto& name : tmpInode.sub) {

    //   res.push_back()
    // }
  }

  void RocksDB::get(Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    Status s;
    string value;
    LOG(INFO) << "get: inode.fullpath: " << inode.fullpath;
    s = get(inode.fullpath, value);
    LOG(INFO) << "get: inode.value(): " << value;
    if (s.IsNotFound() && inode.fullpath != "/") {
      throw runtime_error("path not exist");
    }
    if (s.ok()) {
      auto _json = json::parse(value);
      Inode tmpInode = _json.get<Inode>();
      if (tmpInode.valid == false) {
        throw runtime_error("not valid");
      }
      if (tmpInode.is_directory == true) {
        throw runtime_error("cannot get directory");
      }
      inode = tmpInode;
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
    pathLocks.lock(inode.parent_path());
    pathLocks.lock(inode.key());

    Inode parent_inode = get_parent_inode(inode);
    parent_inode.sub.erase(inode.filename());
    rocksdb::WriteBatch batch;
    batch.Put(parent_inode.fullpath, parent_inode.value());
    batch.Put(inode.fullpath, inode.value());
    s = db_ptr->Write(rocksdb::WriteOptions(), &batch);
    pathLocks.unlock(inode.parent_path());
    pathLocks.unlock(inode.key());
    if (!s.ok()) {
      throw runtime_error("batch write not ok");
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
      throw runtime_error("parent add sub, internal get not ok");
    }
    return parent_inode;
  }

  void RocksDB::internal_put_ok(const Inode& inode) {
    Status s;
    string value;
    pathLocks.lock(inode.parent_path());
    pathLocks.lock(inode.key());
    auto parent_inode = get_parent_inode(inode);
    parent_inode.sub.insert(inode.filename());
    s = get(inode.fullpath, value);
    if (!s.ok()) {
      throw runtime_error("internal get not ok");
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
    pathLocks.unlock(inode.parent_path());
    pathLocks.unlock(inode.key());
    if (!s.ok()) {
      throw runtime_error("batch write not ok");
    }
  }

  void RocksDB::internal_put(const Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    LOG(INFO) << "internal put" << inode.value();
    pathLocks.lock(inode.key());
    Status s = put(inode.fullpath, inode.value());
    pathLocks.unlock(inode.key());
    if (!s.ok()) {
      throw runtime_error("internal put not ok");
    }
  }

  void RocksDB::internal_mkdir(const Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    Status s;
    LOG(INFO) << "internal mkdir" << inode.value();
    pathLocks.lock(inode.parent_path());
    pathLocks.lock(inode.key());
    auto parent_inode = get_parent_inode(inode);
    parent_inode.sub.insert(inode.filename());
    LOG(INFO) << "parent_inode:" << parent_inode.value();
    LOG(INFO) << "curr_inode: " << inode.value();
    rocksdb::WriteBatch batch;
    batch.Put(parent_inode.fullpath, parent_inode.value());
    batch.Put(inode.fullpath, inode.value());
    s = db_ptr->Write(rocksdb::WriteOptions(), &batch);
    pathLocks.unlock(inode.key());
    pathLocks.unlock(inode.parent_path());
    if (!s.ok()) {
      throw runtime_error("batch write not ok");
    }
  };

}  // namespace spkdfs