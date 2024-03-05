#include "common/inode.h"

#include "common/utils.h"
#include "erasurecode.h"
#define max(a, b) (a) > (b) ? (a) : (b)
// #include "erasure_code/ErasureCodeClay.h"

namespace spkdfs {
  using json = nlohmann::json;
  using namespace std;

  std::shared_ptr<StorageType> StorageType::from_string(const std::string& input) {
    std::regex pattern("RS<(\\d+),(\\d+),(\\d+)>|RE<(\\d+),(\\d+)>");
    std::smatch match;
    if (std::regex_match(input, match, pattern)) {
      if (match[1].matched) {
        uint32_t k = std::stoi(match[1]);
        uint32_t m = std::stoi(match[2]);
        uint32_t b = std::stoi(match[3]);
        return std::make_shared<RSStorageType>(k, m, b);
      } else if (match[4].matched) {
        uint32_t replications = std::stoi(match[4]);
        uint32_t b = std::stoi(match[5]);
        return std::make_shared<REStorageType>(replications, b);
      }
    }
    throw runtime_error("cannot solve " + input);
  }

  void from_json(const json& j, std::shared_ptr<StorageType>& ptr) {
    std::string type = j.at("type").get<string>();
    if (type == "RS") {
      uint32_t k = j.at("k").get<uint32_t>();
      uint32_t m = j.at("m").get<uint32_t>();
      uint32_t b = j.at("b").get<uint32_t>();
      ptr = std::make_shared<RSStorageType>(k, m, b);
    } else if (type == "RE") {
      uint32_t replications = j.at("replications").get<uint32_t>();
      uint32_t b = j.at("b").get<uint32_t>();
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
    int instance = liberasurecode_instance_create(EC_BACKEND_JERASURE_RS_CAUCHY, &args);
    char** encoded_data = NULL;
    char** encoded_parity = NULL;
    uint64_t encoded_fragment_len = 0;

    int ret = liberasurecode_encode(instance, data.data(), data.size(), &encoded_data,
                                    &encoded_parity, &encoded_fragment_len);
    if (ret != 0) {
      throw runtime_error("encode error");
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

  void RSStorageType::decode(coro_t::push_type& yield, coro_t::pull_type& generator) const {
    struct ec_args args = {.k = k, .m = m};
    int instance = liberasurecode_instance_create(EC_BACKEND_JERASURE_RS_CAUCHY, &args);
    vector<string> str_datas(k + m);
    int j = 0;
    char* datas[k + m];
    vector<int> missing_idxs;
    for (const auto& str : generator) {
      cout << "sha256sum: " << cal_sha256sum(str) << endl;
      str_datas[j++] = str;
      if (j == k + m) {
        uint64_t len = 0, size = 0;
        for (int i = 0; i < k + m; i++) {
          if (str_datas[i].size()) {
            size = max(size, str_datas[i].size());
            datas[len++] = str_datas[i].data();
          }
        }
        uint64_t out_data_len = 0;
        char* out_data;
        int ret = liberasurecode_decode(instance, datas, len, size, 0, &out_data, &out_data_len);
        if (ret != 0) {
          throw runtime_error("decode error");
        }
        yield(string(out_data, out_data_len));
        j = 0;
      }
    }
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

  void REStorageType::decode(coro_t::push_type& yield, coro_t::pull_type& generator) const {
    cout << "re decode" << endl;
    int i = 0;
    for (auto& str : generator) {
      i++;
      if (i == replications) {
        yield(str);
        i = 0;
      }
    }
  }

  bool REStorageType::check(int success) const { return success >= 1; }
  std::string Inode::value() const {
    nlohmann::json j;
    to_json(j, *this);
    return j.dump();
  }

  void to_json(nlohmann::json& j, const Inode& inode) {
    j = nlohmann::json{{"fullpath", inode.fullpath}, {"is_directory", inode.is_directory},
                       {"filesize", inode.filesize}, {"sub", inode.sub},
                       {"valid", inode.valid},       {"building", inode.building}};
    if (inode.storage_type_ptr != nullptr) {
      nlohmann::json storage_json;
      inode.storage_type_ptr->to_json(storage_json);  // 假设 to_json 返回一个 JSON 对象
      j["storage_type"] = storage_json.dump();  // 转换为字符串并添加到 JSON 对象中
    }
  }

  void from_json(const nlohmann::json& j, Inode& inode) {
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