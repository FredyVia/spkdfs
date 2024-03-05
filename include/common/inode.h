#ifndef INODE_H
#define INODE_H

#include <glog/logging.h>

#include <boost/coroutine2/all.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
#include <string>

namespace spkdfs {
  typedef boost::coroutines2::coroutine<std::string> coro_t;
  class StorageType {
  protected:
    uint32_t b;  // MB

  public:
    StorageType(uint32_t b) : b(b){};
    virtual std::string to_string() const = 0;
    virtual void to_json(nlohmann::json& j) const = 0;
    virtual std::vector<std::string> encode(const std::string& data) const = 0;
    virtual void decode(coro_t::push_type& yield, coro_t::pull_type& generator) const = 0;
    virtual bool check(int success) const = 0;
    static std::shared_ptr<StorageType> from_string(const std::string& input);
    inline virtual uint32_t getBlockSize() { return b * 1024 * 1024; }
    virtual uint32_t getBlocks() = 0;
    virtual ~StorageType(){};
  };
  class RSStorageType : public StorageType {
  public:
    uint32_t k;
    uint32_t m;

    RSStorageType(uint32_t k, uint32_t m, uint32_t b) : k(k), m(m), StorageType(b){};
    std::string to_string() const override;
    void to_json(nlohmann::json& j) const override;
    std::vector<std::string> encode(const std::string& data) const override;
    void decode(coro_t::push_type& yield, coro_t::pull_type& generator) const override;
    bool check(int success) const override;
    inline virtual uint32_t getBlocks() { return k + m; }
  };

  class REStorageType : public StorageType {
  private:
  public:
    uint32_t replications;
    REStorageType(uint32_t replications, uint32_t b) : replications(replications), StorageType(b){};
    std::string to_string() const override;
    void to_json(nlohmann::json& j) const override;
    std::vector<std::string> encode(const std::string& data) const override;
    void decode(coro_t::push_type& yield, coro_t::pull_type& generator) const override;
    bool check(int success) const override;
    inline virtual uint32_t getBlocks() { return replications; }
  };

  inline std::string to_string(std::unique_ptr<StorageType> storageType_ptr) {
    return storageType_ptr->to_string();
  }

  void from_json(const nlohmann::json& j, std::shared_ptr<StorageType>& ptr);

  // inline std::string to_json(std::unique_ptr<StorageType> storageType_ptr) {
  //   return storageType_ptr->to_json();
  // }

  class Inode {
  public:
    std::string fullpath;
    bool is_directory;
    uint32_t filesize;
    std::shared_ptr<StorageType> storage_type_ptr;
    std::set<std::string> sub;  // sub directory for directory or blks for file
    bool valid;
    bool building;
    inline std::string filename() const {
      return std::filesystem::path(fullpath).filename().string();
    }
    inline std::string parent_path() const {
      return std::filesystem::path(fullpath).parent_path().string();
    }
    inline std::string key() const { return fullpath; }
    std::string value() const;
  };

  void to_json(nlohmann::json& j, const Inode& inode);
  // inline std::string to_string(const Inode& inode){

  // }
  void from_json(const nlohmann::json& j, Inode& inode);
}  // namespace spkdfs
#endif