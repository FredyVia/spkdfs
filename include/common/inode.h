#ifndef INODE_H
#define INODE_H

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
#include <string>
namespace spkdfs {
  namespace fs = std::filesystem;
  class StorageType {
  public:
    static std::shared_ptr<StorageType> from_json(const nlohmann::json& j);
    virtual std::string to_string() const = 0;
    virtual void to_json(nlohmann::json& j) const = 0;
  };
  class RSStorageType : public StorageType {
  private:
    unsigned int k;
    unsigned int m;

  public:
    RSStorageType(unsigned int k, unsigned int m) : k(k), m(m){};
    std::string to_string() const override;
    void to_json(nlohmann::json& j) const override;
  };

  class REStorageType : public StorageType {
  private:
    unsigned int replications;

  public:
    REStorageType(unsigned int replications) : replications(replications){};
    std::string to_string() const override;
    void to_json(nlohmann::json& j) const override;
  };

  std::shared_ptr<StorageType> from_string(const std::string& input);

  inline std::string to_string(std::unique_ptr<StorageType> storageType_ptr) {
    return storageType_ptr->to_string();
  }

  std::shared_ptr<StorageType> from_json(const nlohmann::json& j);

  // inline std::string to_json(std::unique_ptr<StorageType> storageType_ptr) {
  //   return storageType_ptr->to_json();
  // }
  class Inode;
  inline void to_json(nlohmann::json& j, const Inode& inode);
  class Inode {
  public:
    std::string fullpath;
    bool is_directory;
    unsigned long long filesize;
    std::shared_ptr<StorageType> storage_type_ptr;
    std::set<std::string> sub;  // sub directory for directory or blks for file
    bool valid;
    bool building;
    inline std::string filename() const {
      fs::path filepath(fullpath);
      return filepath.filename().string();
    }
    inline std::string parent_path() const {
      fs::path filepath(fullpath);
      return filepath.parent_path().string();
    }
    inline std::string key() const { return fullpath; }
    inline std::string value() const {
      nlohmann::json j;
      to_json(j, *this);
      return j.dump();
    }
  };

  inline void to_json(nlohmann::json& j, const Inode& inode) {
    j = nlohmann::json{{"fullpath", inode.fullpath}, {"is_directory", inode.is_directory},
                       {"filesize", inode.filesize}, {"sub", inode.sub},
                       {"valid", inode.valid},       {"building", inode.building}};
    if (inode.storage_type_ptr != nullptr) {
      nlohmann::json storage_json;
      inode.storage_type_ptr->to_json(storage_json);  // 假设 to_json 返回一个 JSON 对象
      j["storage_type"] = storage_json.dump();  // 转换为字符串并添加到 JSON 对象中
    }
  }
  // inline std::string to_string(const Inode& inode){

  // }
  inline void from_json(const nlohmann::json& j, Inode& inode) {
    inode.fullpath = j.at("fullpath").get<std::string>();
    inode.is_directory = j.at("is_directory").get<bool>();
    inode.filesize = j.at("filesize").get<int>();
    if (j.find("storage_type") != j.end()) {
      inode.storage_type_ptr = StorageType::from_json(j.at("storage_type"));
    } else {
      inode.storage_type_ptr = nullptr;
    }
    inode.sub = j.at("sub").get<std::set<std::string>>();
    inode.valid = j.at("valid").get<bool>();
    inode.building = j.at("building").get<bool>();
  }
}  // namespace spkdfs
#endif