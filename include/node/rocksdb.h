#ifndef ROCKSDB_H
#define ROCKSDB_H
#include <mutex>
#include <string>

#include "common/inode.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/backup_engine.h"

namespace spkdfs {

  class RocksDB {
  private:
    std::string origin_dir;
    std::string backup_dir;
    rocksdb::DB* db_ptr = nullptr;
    rocksdb::BackupEngine* backup_engine_ptr;
    rocksdb::Status get(const std::string& key, std::string& value);
    rocksdb::Status put(const std::string& key, const std::string& value);
    void open();
    void close();

  public:
    RocksDB(const std::string& db_dir);
    ~RocksDB();
    void snapshot();
    void load_snapshot();
    void ls(Inode& inode);
    void prepare_mkdir(Inode& inode);
    void internal_mkdir(const Inode& inode);
  };
}  // namespace spkdfs
#endif