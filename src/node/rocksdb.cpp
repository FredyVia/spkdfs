#include "node/rocksdb.h"

#include <glog/logging.h>
#include <rocksdb/write_batch.h>

#include <nlohmann/json.hpp>
#include <string>

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
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
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
  rocksdb::Status RocksDB::get(const std::string& key, std::string& value) {
    // private not check
    // if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");

    return db_ptr->Get(rocksdb::ReadOptions(), key, &value);
  }
  // std::vector<Status> RocksDB::multiGet(const vector<std::string>& keys,
  //                                       std::vector<std::string>& values) {
  //   // private not check
  //   // if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
  //   std::vector<Slice> keys;
  //   std::vector<PinnableSlice> values;

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
    string value;
    Status s = get(inode.fullpath, value);
    if (s.IsNotFound()) {
      throw runtime_error("path not exist");
    }
    auto _json = json::parse(value);
    Inode tmpInode = _json.get<Inode>();
    if (tmpInode.valid == false) {
      throw runtime_error("path not exist");
    }
    // vector<Inode> res;
    // for (const auto& name : tmpInode.sub) {

    //   res.push_back()
    // }
    inode = tmpInode;
  }
  void RocksDB::prepare_mkdir(Inode& inode) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");

    if (inode.fullpath == "/") throw runtime_error("path already exists");
    Status s;
    string value;
    Inode tmpInode;
    // check parent_path
    string parent_path = inode.parent_path();
    DLOG(INFO) << "parent_path: " << parent_path;
    if (parent_path != "/") {
      s = get(parent_path, value);
      if (s.IsNotFound()) {
        throw runtime_error("parent path not exist");
      }
      auto _json = json::parse(value);
      tmpInode = _json.get<Inode>();
      if (tmpInode.valid == false) {
        throw runtime_error("parent path not exist");
      }
    }
    s = get(inode.fullpath, value);
    if (s.ok()) {
      auto _json = json::parse(value);
      tmpInode = _json.get<Inode>();
      if (tmpInode.valid == true) {
        throw runtime_error("path already exists");
      }
    }
    inode.is_directory = true;
    inode.filesize = 0;
    inode.storage_type = StorageType::STORAGETYPE_REPLICA;
    inode.sub = {};
    inode.valid = true;
    inode.building = false;
  }
  void RocksDB::internal_mkdir(const Inode& inode) {
    // RocksDB::put(const std::string& key, const std::string& value) {
    if (db_ptr == nullptr) throw runtime_error(string(__func__) + "db not ready");
    string value;
    Status s = get(inode.parent_path(), value);
    Inode parent_inode;
    if (s.IsNotFound()) {
      // parent_path must be "/"
      if (inode.parent_path() != "/") {
        LOG(ERROR) << "ERROR internal_mkdir: " << inode.fullpath;
        throw runtime_error(string(__func__) + " parent_path != '/'");
      }
      parent_inode.fullpath = "/";
    } else {
      auto _json = json::parse(value);
      parent_inode = _json.get<Inode>();
    }
    parent_inode.sub.push_back(inode.fullpath);
    LOG(INFO) << "parent_inode:" << parent_inode.value();
    LOG(INFO) << "curr_inode: " << inode.value();
    rocksdb::WriteBatch batch;
    batch.Put(parent_inode.fullpath, parent_inode.value());
    batch.Put(inode.fullpath, inode.value());
    s = db_ptr->Write(rocksdb::WriteOptions(), &batch);
  };

}  // namespace spkdfs