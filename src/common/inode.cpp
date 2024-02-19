#include "common/inode.h"

#include "common/mln_rs.h"

namespace spkdfs {
  using json = nlohmann::json;
  using namespace std;
  std::shared_ptr<StorageType> StorageType::from_string(const std::string& input) {
    std::regex pattern("RS<(\\d+),(\\d+)>|RE<(\\d+)>");
    std::smatch match;
    if (std::regex_match(input, match, pattern)) {
      if (match[1].matched) {
        uint32_t k = std::stoi(match[1]);
        uint32_t m = std::stoi(match[2]);
        return std::make_shared<RSStorageType>(k, m);
      } else if (match[3].matched) {
        uint32_t replications = std::stoi(match[3]);
        return std::make_shared<REStorageType>(replications);
      }
    }
    throw runtime_error("cannot decode" + input);
    return nullptr;
  }

  void from_json(const json& j, std::shared_ptr<StorageType>& ptr) {
    std::string type = j.at("type").get<string>();
    if (type == "RS") {
      uint32_t k = j.at("k").get<uint32_t>();
      uint32_t m = j.at("m").get<uint32_t>();
      ptr = std::make_shared<RSStorageType>(k, m);
    } else if (type == "RE") {
      uint32_t replications = j.at("replications").get<uint32_t>();
      ptr = std::make_shared<REStorageType>(replications);
    } else {
      LOG(ERROR) << type;
      throw std::invalid_argument("Unknown StorageType");
    }
  }

  std::string RSStorageType::to_string() const {
    return "RS<" + std::to_string(k) + "," + std::to_string(m) + ">";
  }
  void RSStorageType::to_json(json& j) const {
    j["type"] = "RS";
    j["k"] = k;
    j["m"] = m;
  }
  std::vector<std::string> RSStorageType::encode(const std::string& data) const {
    std::vector<std::string> vec;
    // 使用Melon库进行纠删码编码
    mln_rs_result_t* res = mln_rs_encode((uint8_t*)data.data(), data.size() / k, k, m);
    if (res == nullptr) {
      cerr << "rs encode failed.\n";
      throw runtime_error("rs encode failed");
    }

    for (int j = 0; j < k + m; j++) {
      uint8_t* encodedData = mln_rs_result_get_data_by_index(res, j);
      if (encodedData == nullptr) {
        throw runtime_error("encoded error");
      }
      vec.push_back(string((char*)encodedData));
    }
    mln_rs_result_free(res);
    return vec;
  }

  void RSStorageType::decode(coro_t::push_type& yield, coro_t::pull_type& generator) const {
    int j = 0;
    uint8_t* datas[k + m];
    for (auto& str : generator) {
      datas[j] = (uint8_t*)str.data();

      j++;
      if (j == k + m) {
        string res;
        mln_rs_result_t* dres = mln_rs_decode(datas, str.size(), k, m);
        if (dres == NULL) {
          throw runtime_error("decode error");
        }
        for (int i = 0; i < k; i++) {
          res.append((char*)mln_rs_result_get_data_by_index(dres, i));
        }
        yield(res);
        mln_rs_result_free(dres);
        res.clear();
        j = 0;
      }
    }
  }

  bool RSStorageType::check(int success) const { return success > k; }

  std::vector<std::string> REStorageType::encode(const std::string& data) const {
    vector<std::string> vec;
    for (int i = 0; i < replications; i++) {
      vec.push_back(data);
    }
    return vec;
  }

  std::string REStorageType::to_string() const {
    return "RE<" + std::to_string(replications) + ">";
  }
  void REStorageType::to_json(json& j) const {
    j["type"] = "RE";
    j["replications"] = replications;
  }

  void REStorageType::decode(coro_t::push_type& yield, coro_t::pull_type& generator) const {
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
}  // namespace spkdfs