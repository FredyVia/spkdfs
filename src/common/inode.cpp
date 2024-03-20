#include "common/inode.h"

#include "common/config.h"
#include "common/exception.h"
#include "common/utils.h"
#include "erasurecode.h"
#include "service.pb.h"
// #include "erasure_code/ErasureCodeClay.h"
namespace spkdfs {
  using json = nlohmann::json;
  using namespace std;

  std::shared_ptr<StorageType> StorageType::from_string(const std::string& input) {
    std::regex pattern("RS<(\\d+),(\\d+),(\\d+)>|RE<(\\d+),(\\d+)>");
    std::smatch match;
    if (std::regex_match(input, match, pattern)) {
      if (match[1].matched) {
        int k = std::stoi(match[1]);
        int m = std::stoi(match[2]);
        int b = std::stoi(match[3]);
        return std::make_shared<RSStorageType>(k, m, b);
      } else if (match[4].matched) {
        int replications = std::stoi(match[4]);
        int b = std::stoi(match[5]);
        return std::make_shared<REStorageType>(replications, b);
      }
    }
    throw runtime_error("cannot solve " + input);
  }

  void from_json(const json& j, std::shared_ptr<StorageType>& ptr) {
    std::string type = j.at("type").get<string>();
    if (type == "RS") {
      int k = j.at("k").get<int>();
      int m = j.at("m").get<int>();
      int b = j.at("b").get<int>();
      ptr = std::make_shared<RSStorageType>(k, m, b);
    } else if (type == "RE") {
      int replications = j.at("replications").get<int>();
      int b = j.at("b").get<int>();
      ptr = std::make_shared<REStorageType>(replications, b);
    } else {
      LOG(ERROR) << type;
      throw std::invalid_argument("Unknown StorageType");
    }
  }

  std::string RSStorageType::to_string() const {
    return "RS<" + std::to_string(k) + "," + std::to_string(m) + "," + std::to_string(b) + ">";
  }

  void RSStorageType::to_json(json& j) const {
    j["type"] = "RS";
    j["k"] = k;
    j["m"] = m;
    j["b"] = b;
  }

  std::vector<std::string> RSStorageType::encode(const std::string& data) const {
    struct ec_args args = {.k = k, .m = m};
    int instance = liberasurecode_instance_create(EC_BACKEND_ISA_L_RS_VAND, &args);
    char** encoded_data = NULL;
    char** encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;

    int ret = liberasurecode_encode(instance, data.data(), data.size(), &encoded_data,
                                    &encoded_parity, &encoded_fragment_len);
    if (ret != 0) {
      liberasurecode_instance_destroy(instance);
      throw runtime_error("encode error, error code: " + std::to_string(ret));
    }
    vector<string> res(k + m, string(encoded_fragment_len, ' '));
    for (int i = 0; i < k; i++) {
      res[i] = string(encoded_data[i], encoded_fragment_len);
    }
    for (int i = 0; i < m; i++) {
      res[k + i] = string(encoded_parity[i], encoded_fragment_len);
    }

    liberasurecode_encode_cleanup(instance, encoded_data, encoded_parity);
    liberasurecode_instance_destroy(instance);
    return res;
  }

  std::string RSStorageType::decode(vector<std::string> vec) const {
    assert(vec.size() >= k);
    assert(vec.size() <= k + m);
    struct ec_args args = {.k = k, .m = m};
    int instance = liberasurecode_instance_create(EC_BACKEND_ISA_L_RS_VAND, &args);
    int len = 0;
    char* datas[k];
    for (auto& str : vec) {
      cout << "sha256sum: " << cal_sha256sum(str) << endl;
      datas[len++] = str.data();
      if (len == k) {
        break;
      }
    }
    uint64_t out_data_len = 0;
    char* out_data;
    int ret
        = liberasurecode_decode(instance, datas, len, vec[0].size(), 0, &out_data, &out_data_len);
    if (ret != 0) {
      liberasurecode_instance_destroy(instance);
      throw runtime_error("decode error");
    }
    return string(out_data, out_data_len);
  }

  bool RSStorageType::check(int success) const { return success > k; }

  std::vector<std::string> REStorageType::encode(const std::string& data) const {
    cout << "re encode" << endl;
    vector<std::string> vec;
    for (int i = 0; i < replications; i++) {
      vec.push_back(data);
    }
    return vec;
  }

  std::string REStorageType::to_string() const {
    return "RE<" + std::to_string(replications) + "," + std::to_string(b) + ">";
  }

  void REStorageType::to_json(json& j) const {
    j["type"] = "RE";
    j["replications"] = replications;
    j["b"] = b;
  }
  std::string REStorageType::decode(std::vector<std::string> vec) const { return vec.front(); }

  bool REStorageType::check(int success) const { return success >= 1; }

  std::string Inode::value() const {
    nlohmann::json j;
    to_json(j, *this);
    return j.dump();
  }

  void Inode::lock() {
    uint64_t now = get_time();
    if (ddl_lock >= now) {
      throw MessageException(EXPECTED_LOCK_CONFLICT,
                             fullpath + " file is being edit! ddl_lock: " + std::to_string(ddl_lock)
                                 + ", now: " + std::to_string(now));
    }
    ddl_lock = now + LOCK_REFRESH_INTERVAL;
  }

  void Inode::unlock() { ddl_lock = 0; }

  void Inode::update_ddl_lock() { ddl_lock = get_time() + LOCK_REFRESH_INTERVAL; }

  std::string Inode::filename() const {
    std::string s = std::filesystem::path(fullpath).filename().string();
    if (is_directory) {
      s += "/";
    }
    return s;
  }

  std::string Inode::parent_path() const {
    return std::filesystem::path(fullpath).parent_path().string();
  }

  inline std::string to_string(std::unique_ptr<StorageType> storageType_ptr) {
    return storageType_ptr->to_string();
  }

  Inode Inode::get_default_dir(const string& path) {
    Inode inode;
    inode.set_fullpath(path);
    inode.is_directory = true;
    inode.filesize = 0;
    inode.storage_type_ptr = nullptr;
    inode.sub = {};
    inode.valid = true;
    inode.ddl_lock = 0;
    return inode;
  }

  std::string encode_one_sub(const std::vector<std::pair<std::string, std::string>>& nodes_hashs) {
    string res;
    for (int i = 0; i < nodes_hashs.size(); i++) {
      std::ostringstream oss;
      res += to_string(nodes_hashs[i].first) + "|" + nodes_hashs[i].second + ",";
    }
    res.pop_back();
    // LOG(INFO) << res;
    return res;
  }

  std::vector<std::pair<std::string, std::string>> decode_one_sub(const std::string& one_sub) {
    std::vector<std::pair<std::string, std::string>> res;
    std::map<int, std::pair<std::string, std::string>> tmp;
    stringstream ss(one_sub);
    string str;
    while (getline(ss, str, ',')) {
      stringstream ss(str);
      string node, blkid;
      getline(ss, node, '|');   // 提取第一个部分
      getline(ss, blkid, '|');  // 提取第二个部分
      cout << node << "|" << blkid << endl;
      res.push_back(make_pair(node, blkid));
    }
    return res;
  }

  void to_json(nlohmann::json& j, const Inode& inode) {
    j = nlohmann::json{{"fullpath", inode.get_fullpath()},
                       {"is_directory", inode.is_directory},
                       {"filesize", inode.filesize},
                       {"sub", inode.sub},
                       {"valid", inode.valid},
                       {"ddl_lock", inode.ddl_lock}};
    //  ,{"modification_time", inode.modification_time}

    if (inode.storage_type_ptr != nullptr) {
      nlohmann::json storage_json;
      inode.storage_type_ptr->to_json(storage_json);  // 假设 to_json 返回一个 JSON 对象
      j["storage_type"] = storage_json.dump();  // 转换为字符串并添加到 JSON 对象中
    }
  }

  void from_json(const nlohmann::json& j, Inode& inode) {
    inode.set_fullpath(j.at("fullpath").get<std::string>());
    inode.is_directory = j.at("is_directory").get<bool>();
    inode.filesize = j.at("filesize").get<uint64_t>();
    inode.storage_type_ptr = nullptr;
    if (j.find("storage_type") != j.end()) {
      // LOG(INFO) << j.at("storage_type");
      auto storage_type_json = nlohmann::json::parse(j.at("storage_type").get<std::string>());
      from_json(storage_type_json, inode.storage_type_ptr);
    }
    inode.sub = j.at("sub").get<std::vector<std::string>>();
    inode.valid = j.at("valid").get<bool>();
    inode.ddl_lock = j.at("ddl_lock").get<uint64_t>();
    // inode.modification_time = j.at("modification_time").get<int>();
  }

  bool operator==(const Inode& linode, const Inode& rinode) noexcept {
    return linode.sub == rinode.sub && linode.filesize == rinode.filesize;
  }
}  // namespace spkdfs