#include "common/inode.h"

#include "common/utils.h"
#include "erasurecode.h"
#define max(a, b) (a) > (b) ? (a) : (b)
// #include "erasure_code/ErasureCodeClay.h"

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
  // std::vector<std::string> RSStorageType::encode(const std::string& _data) const {
  //   cout << "rs encode" << endl;
  //   string data = _data;
  //   data.resize(((_data.size() + k - 1) / k) * k, '0');
  //   std::vector<std::string> vec;
  //   // 使用Melon库进行纠删码编码
  //   mln_rs_result_t* res = mln_rs_encode((uint8_t*)data.data(), data.size() / k, k, m);
  //   if (res == nullptr) {
  //     cerr << "rs encode failed.\n";
  //     throw runtime_error("rs encode failed");
  //   }

  //   for (int j = 0; j < k + m; j++) {
  //     uint8_t* encodedData = mln_rs_result_get_data_by_index(res, j);
  //     if (encodedData == nullptr) {
  //       throw runtime_error("encoded error");
  //     }
  //     cout << "encodedData: " << string((char*)encodedData) << endl;
  //     vec.push_back(string((char*)encodedData));
  //   }
  //   mln_rs_result_free(res);
  //   return vec;
  // }
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
  // void RSStorageType::decode(coro_t::push_type& yield, coro_t::pull_type& generator) const {
  //   cout << "rs decode" << endl;
  //   int j = 0;
  //   uint8_t* datas[k + m];
  //   vector<string> str_datas(k + m);
  //   int size = 0;
  //   for (auto& str : generator) {
  //     str_datas[j] = str;
  //     size = max(size, str.size());
  //     j++;
  //     if (j == k + m) {
  //       for (int i = 0; i < k + m; i++) {
  //         cout << "str_datas: " << str_datas[i] << endl;
  //         datas[i] = str_datas[i].size() ? (uint8_t*)str_datas[i].data() : NULL;
  //       }
  //       mln_rs_result_t* dres = mln_rs_decode(datas, size, k, m);
  //       if (dres == NULL) {
  //         throw runtime_error("decode error");
  //       }
  //       string res;
  //       for (int i = 0; i < k; i++) {
  //         auto str = string((char*)mln_rs_result_get_data_by_index(dres, i));
  //         res.append(str);
  //       }
  //       res.resize(b);
  //       yield(res);
  //       mln_rs_result_free(dres);
  //       res.clear();
  //       j = 0;
  //     }
  //   }
  // }
  // void RSStorageType::decode(coro_t::push_type& yield, coro_t::pull_type& generator) const {}
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
    return "RE<" + std::to_string(replications) + ">";
  }
  void REStorageType::to_json(json& j) const {
    j["type"] = "RE";
    j["replications"] = replications;
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
}  // namespace spkdfs