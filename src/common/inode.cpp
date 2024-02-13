#include "common/inode.h"

namespace spkdfs {
  using json = nlohmann::json;
  std::string RSStorageType::to_string() const {
    return "RS(" + std::to_string(k) + "," + std::to_string(m) + ")";
  }
  void RSStorageType::to_json(json& j) const {
    j["type"] = "RS";
    j["k"] = k;
    j["m"] = m;
  }

  std::string REStorageType::to_string() const { return "RE(" + std::to_string(replications) + ")"; }
  void REStorageType::to_json(json& j) const {
    j["type"] = "RE";
    j["replications"] = replications;
  }

  std::shared_ptr<StorageType> from_string(const std::string& input) {
    std::regex pattern("RS\\((\\d+),(\\d+)\\)|RE\\((\\d+)\\)");
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
    return nullptr;
  }

  std::shared_ptr<StorageType> StorageType::from_json(const json& j) {
    std::string type = j.at("type");
    if (type == "RS") {
      uint32_t k = j.at("k");
      uint32_t m = j.at("m");
      return std::make_shared<RSStorageType>(k, m);
    } else if (type == "RE") {
      uint32_t replications = j.at("replications");
      return std::make_shared<REStorageType>(replications);
    } else {
      throw std::invalid_argument("Unknown StorageType");
    }
  }

}  // namespace spkdfs