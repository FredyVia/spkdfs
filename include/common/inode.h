#ifndef INODE_H
#define INODE_H

#include <sqlite_orm/sqlite_orm.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace spkdfs {

  enum class StorageType { STORAGETYPE_RS = 0, STORAGETYPE_REPLICA = 1 };
  // #define STORAGETYPE_RS StorageType.STORAGETYPE_RS
  // #define STORAGETYPE_REPLICA StorageType.STORAGETYPE_REPLICA
  inline std::unique_ptr<StorageType> from_string(const std::string& s) {
    if (s == "STORAGETYPE_RS") {
      return std::make_unique<StorageType>(StorageType::STORAGETYPE_RS);
    } else if (s == "STORAGETYPE_REPLICA") {
      return std::make_unique<StorageType>(StorageType::STORAGETYPE_REPLICA);
    }
    return nullptr;
  }
  inline std::string to_string(StorageType st) {
    switch (st) {
      case StorageType::STORAGETYPE_RS:
        return "STORAGETYPE_RS";
      case StorageType::STORAGETYPE_REPLICA:
        return "STORAGETYPE_REPLICA";
    }
    throw std::domain_error("Invalid StorageType enum");
  }
  class Inode {
  public:
    long file_id;
    std::string parent_path;
    std::string filename;
    std::string fullpath;
    bool is_directory;
    int filesize;
    StorageType storage_type;
    std::vector<std::string> blk_ids;
    bool valid;
    bool building;
  };

  inline void to_json(nlohmann::json& j, const Inode& inode) {
    j = nlohmann::json{{"file_id", inode.file_id},
                       {"parent_path", inode.parent_path},
                       {"filename", inode.filename},
                       {"fullpath", inode.fullpath},
                       {"is_directory", inode.is_directory},
                       {"filesize", inode.filesize},
                       {"storage_type", inode.storage_type},
                       {"blk_ids", inode.blk_ids},
                       {"valid", inode.valid},
                       {"building", inode.building}};
  }
  // inline std::string to_string(const Inode& inode){

  // }
  inline void from_json(const nlohmann::json& j, Inode& inode) {
    inode.parent_path = j.at("parent_path").get<std::string>();
    inode.filename = j.at("filename").get<std::string>();
    inode.fullpath = j.at("fullpath").get<std::string>();
    inode.is_directory = j.at("is_directory").get<bool>();
    inode.filesize = j.at("filesize").get<int>();
    j.at("storage_type").get_to(inode.storage_type);
    inode.blk_ids = j.at("blk_ids").get<std::vector<std::string>>();
    inode.valid = j.at("valid").get<bool>();
    inode.building = j.at("building").get<bool>();
  }
}  // namespace spkdfs
namespace sqlite_orm {
  template <> struct type_printer<spkdfs::StorageType> : public text_printer {};
  template <> struct statement_binder<spkdfs::StorageType> {
    int bind(sqlite3_stmt* stmt, int index, const spkdfs::StorageType& value) {
      return statement_binder<std::string>().bind(stmt, index, spkdfs::to_string(value));
    }
  };
  template <> struct field_printer<spkdfs::StorageType> {
    std::string operator()(const spkdfs::StorageType& t) const { return spkdfs::to_string(t); }
  };
  template <> struct row_extractor<spkdfs::StorageType> {
    spkdfs::StorageType extract(const char* row_value) {
      if (auto st = spkdfs::from_string(row_value)) {
        return *st;
      } else {
        throw std::runtime_error("incorrect st string (" + std::string(row_value) + ")");
      }
    }

    spkdfs::StorageType extract(sqlite3_stmt* stmt, int columnIndex) {
      auto str = sqlite3_column_text(stmt, columnIndex);
      return this->extract((const char*)str);
    }
  };
}  // namespace sqlite_orm
#endif