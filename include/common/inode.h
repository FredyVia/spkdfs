#ifndef INODE_H
#define INODE_H

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace spkdfs {
  namespace fs = std::filesystem;
  using json = nlohmann::json;

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
  class Inode;
  inline void to_json(nlohmann::json& j, const Inode& inode);
  class Inode {
  public:
    std::string fullpath;
    bool is_directory;
    unsigned long long filesize;
    StorageType storage_type;
    std::vector<std::string> sub;  // sub directory for directory or blks for file
    bool valid;
    bool building;
    inline std::string parent_path() const {
      fs::path filepath(fullpath);
      return filepath.parent_path().string();
    }
    inline std::string key() const { return fullpath; }
    inline std::string value() const {
      json j;
      to_json(j, *this);
      return j.dump();
    }
  };

  inline void to_json(nlohmann::json& j, const Inode& inode) {
    j = nlohmann::json{{"fullpath", inode.fullpath}, {"is_directory", inode.is_directory},
                       {"filesize", inode.filesize}, {"storage_type", inode.storage_type},
                       {"sub", inode.sub},           {"valid", inode.valid},
                       {"building", inode.building}};
  }
  // inline std::string to_string(const Inode& inode){

  // }
  inline void from_json(const nlohmann::json& j, Inode& inode) {
    inode.fullpath = j.at("fullpath").get<std::string>();
    inode.is_directory = j.at("is_directory").get<bool>();
    inode.filesize = j.at("filesize").get<int>();
    j.at("storage_type").get_to(inode.storage_type);
    inode.sub = j.at("sub").get<std::vector<std::string>>();
    inode.valid = j.at("valid").get<bool>();
    inode.building = j.at("building").get<bool>();
  }
}  // namespace spkdfs
#endif