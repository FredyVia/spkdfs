#ifndef ROCKSDB_H
#define ROCKSDB_H
#include <mutex>
#include <shared_mutex>
#include <string>

#include "common/inode.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/backup_engine.h"
namespace spkdfs {

  class PathLocks {
  private:
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> locks;
    mutable std::shared_mutex mapMutex;  // 用于保护锁映射的互斥量

  public:
    void lock(const std::string& path);

    void unlock(const std::string& path);
  };
  class RocksDB {
  private:
    std::string origin_dir;
    std::string backup_dir;
    rocksdb::DB* db_ptr = nullptr;
    rocksdb::BackupEngine* backup_engine_ptr;
    PathLocks pathLocks;
    rocksdb::Status get(const std::string& key, std::string& value) const;
    rocksdb::Status put(const std::string& key, const std::string& value);
    void open();
    void close();
    void try_to_add(const Inode& inode) const;
    void try_to_rm(const Inode& inode) const;
    bool path_exists(const std::string& path) const;
    Inode get_parent_inode(const Inode& inode);

  public:
    RocksDB(const std::string& db_dir);
    ~RocksDB();
    void snapshot();
    void load_snapshot();
    void ls(Inode& inode);
    void get(Inode& inode);
    void prepare_mkdir(Inode& inode);
    void prepare_rm(Inode& inode);
    void prepare_put(Inode& inode);
    void prepare_put_ok(Inode& inode);
    void internal_mkdir(const Inode& inode);
    void internal_rm(const Inode& inode);
    void internal_put(const Inode& inode);
    void internal_put_ok(const Inode& inode);
  };
}  // namespace spkdfs
#endif