#ifndef INODE_H
#define INODE_H

#include <glog/logging.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
// #include <mutex>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
// #include <shared_mutex>
#include <string>

#include "common/utils.h"

namespace spkdfs {
  class StorageType {
  protected:
    int b;  // MB

  public:
    StorageType(int b) : b(b){};
    virtual std::string to_string() const = 0;
    virtual void to_json(nlohmann::json& j) const = 0;
    virtual std::vector<std::string> encode(const std::string& data) const = 0;
    virtual std::string decode(std::vector<std::string> vec) const = 0;
    virtual bool check(int success) const = 0;
    static std::shared_ptr<StorageType> from_string(const std::string& input);
    inline virtual int getBlockSize() { return b * 1024 * 1024; }
    virtual int getBlocks() = 0;
    virtual int getDecodeBlocks() = 0;
    virtual ~StorageType(){};
  };
  class RSStorageType : public StorageType {
  public:
    int k;
    int m;

    RSStorageType(int k, int m, int b) : k(k), m(m), StorageType(b){};
    std::string to_string() const override;
    void to_json(nlohmann::json& j) const override;
    std::vector<std::string> encode(const std::string& data) const override;
    std::string decode(std::vector<std::string> vec) const override;
    bool check(int success) const override;
    // inline virtual int getBlockSize() { return b * 1024 * 1024; }
    inline virtual int getBlocks() { return k + m; }
    virtual int getDecodeBlocks() { return k; }
  };

  class REStorageType : public StorageType {
  private:
  public:
    int replications;
    REStorageType(int replications, int b) : replications(replications), StorageType(b){};
    std::string to_string() const override;
    void to_json(nlohmann::json& j) const override;
    std::vector<std::string> encode(const std::string& data) const override;
    std::string decode(std::vector<std::string> vec) const override;
    bool check(int success) const override;
    inline virtual int getBlocks() { return replications; }
    virtual int getDecodeBlocks() { return 1; }
  };

  inline std::string to_string(std::unique_ptr<StorageType> storageType_ptr) {
    return storageType_ptr->to_string();
  }

  void from_json(const nlohmann::json& j, std::shared_ptr<StorageType>& ptr);

  // inline std::string to_json(std::unique_ptr<StorageType> storageType_ptr) {
  //   return storageType_ptr->to_json();
  // }
  std::vector<std::pair<std::string, std::string>> decode_one_sub(const std::string& str);
  std::string encode_one_sub(const std::vector<std::pair<std::string, std::string>>& sub);
  class Inode {
    std::string fullpath;

  public:
    inline void set_fullpath(const std::string& fullpath) {
      this->fullpath = simplify_path(fullpath);
    }
    inline std::string get_fullpath() const { return fullpath; }
    bool is_directory = false;
    uint64_t filesize = 0;
    std::shared_ptr<StorageType> storage_type_ptr;
    std::vector<std::string> sub;
    bool valid;
    bool building;
    // int modification_time;
    inline int getBlockSize() const {
      return storage_type_ptr == nullptr ? 0 : storage_type_ptr->getBlockSize();
    }
    inline std::string filename() const {
      std::string s = std::filesystem::path(fullpath).filename().string();
      if (is_directory) {
        s += "/";
      }
      return s;
    }
    inline std::string parent_path() const {
      return std::filesystem::path(fullpath).parent_path().string();
    }
    inline std::string key() const { return fullpath; }
    std::string value() const;
  };

  void to_json(nlohmann::json& j, const Inode& inode);

  void from_json(const nlohmann::json& j, Inode& inode);
}  // namespace spkdfs
#endif