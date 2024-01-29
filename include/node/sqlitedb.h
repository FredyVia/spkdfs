#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <sqlite_orm/sqlite_orm.h>

#include <memory>
#include <string>
#include <vector>

#include "common/block.h"
#include "common/inode.h"

namespace spkdfs {

  inline auto _initStorage(const std::string& db_path) {
    using namespace sqlite_orm;
    auto storage_ = make_storage(
        db_path,
        make_table("block", make_column("blk_id", &Block::blk_id, primary_key()),
                   make_column("node", &Block::node)),
        make_table("inode", make_column("file_id", &Inode::file_id, primary_key().autoincrement()),
                   make_column("parent_path", &Inode::parent_path),
                   make_column("filename", &Inode::filename),
                   make_column("fullpath", &Inode::fullpath),
                   make_column("is_directory", &Inode::is_directory),
                   make_column("filesize", &Inode::filesize),
                   make_column("storage_type", &Inode::storage_type),
                   make_column("valid", &Inode::valid), make_column("building", &Inode::building)));
    //  make_column("blk_ids", &Inode::blk_ids)));
    storage_.sync_schema();
    storage_.open_forever();
    return storage_;
  }
  using Storage = decltype(_initStorage(""));

  // // 数据库操作函数
  // // int addInode(const Inode& inode);

  class SqliteDB {
  private:
    std::unique_ptr<Storage> storage_ptr;

  public:
    SqliteDB();
    std::vector<Inode> ls(const Inode& inode);
    void prepare_mkdir(Inode& inode);
    std::vector<int> get(const Inode& inode);
    void prepare_put(Inode& inode);

    void internal_put(const Inode& inode);
    void internal_mkdir(const Inode& inode);
  };

}  // namespace spkdfs

#endif  // SQLITEDB_H
