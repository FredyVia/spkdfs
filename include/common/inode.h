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
    StorageType(unsigned int b) : b(b) {}
    unsigned int b;

  public:
    virtual inline unsigned int get_b() { return b * 1024 * 1024; }
    virtual std::string to_string() const = 0;
    virtual void to_json(nlohmann::json& j) const = 0;
    virtual std::vector<std::string> encode(const std::string& data) const = 0;
    virtual void decode(coro_t::push_type& yield, coro_t::pull_type& generator) const = 0;
    virtual bool check(int success) const = 0;
    static std::shared_ptr<StorageType> from_string(const std::string& input);
    virtual ~StorageType(){};
  };
  // std::vector<string> encode(StorageType* storage_type_ptr, std::istream ist) {
  //   while (file.read(&block[0], storage_type_ptr->get_b()) || file.gcount() > 0) {
  //     int succ = 0;
  //     size_t bytesRead = file.gcount();
  //     auto vec = storage_type_ptr->encode(block);
  //     // 上传每个编码后的分块
  //     blockIndex++;
  //   }
  // }
  class RSStorageType : public StorageType {
  public:
    unsigned int k;
    unsigned int m;

    RSStorageType(unsigned int k, unsigned int m, unsigned int b) : StorageType(b), k(k), m(m){};
    inline unsigned int get_b() override { return ((StorageType::get_b() + k - 1) / k) * k; };
    std::string to_string() const override;
    void to_json(nlohmann::json& j) const override;
    std::vector<std::string> encode(const std::string& data) const override;
    void decode(coro_t::push_type& yield, coro_t::pull_type& generator) const override;
    bool check(int success) const override;
  };

  class REStorageType : public StorageType {
  private:
  public:
    unsigned int replications;
    REStorageType(unsigned int replications, unsigned int b)
        : StorageType(b), replications(replications){};
    std::string to_string() const override;
    void to_json(nlohmann::json& j) const override;
    std::vector<std::string> encode(const std::string& data) const override;
    void decode(coro_t::push_type& yield, coro_t::pull_type& generator) const override;
    bool check(int success) const override;
  };

  inline std::string to_string(std::unique_ptr<StorageType> storageType_ptr) {
    return storageType_ptr->to_string();
  }
  void from_json(const nlohmann::json& j, std::shared_ptr<StorageType>& ptr);

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
      std::filesystem::path filepath(fullpath);
      return filepath.filename().string();
    }
    inline std::string parent_path() const {
      std::filesystem::path filepath(fullpath);
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
    inode.storage_type_ptr = nullptr;
    if (j.find("storage_type") != j.end()) {
      LOG(INFO) << j.at("storage_type");
      auto storage_type_json = nlohmann::json::parse(j.at("storage_type").get<std::string>());
      from_json(storage_type_json, inode.storage_type_ptr);
    }
    inode.sub = j.at("sub").get<std::set<std::string>>();
    inode.valid = j.at("valid").get<bool>();
    inode.building = j.at("building").get<bool>();
  }
}  // namespace spkdfs
#endif