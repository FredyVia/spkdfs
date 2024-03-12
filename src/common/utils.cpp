#include "common/utils.h"

#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/sha.h>
#include <glog/logging.h>

#include <filesystem>

namespace spkdfs {
  using namespace std;

  void createDirectoryIfNotExist(const std::string& dir) {
    filesystem::path dirPath{dir};
    error_code ec;  // 用于接收错误码

    // 创建目录，包括所有必要的父目录
    if (!filesystem::create_directories(dirPath, ec)) {
      if (ec) {
        LOG(ERROR) << "Error: Unable to create directory " << dir << ". " << ec.message();
        throw runtime_error("mkdir error: " + ec.message());
      }
    }
  }
  std::string cal_sha256sum(std::string data) {
    string sha256sum;
    CryptoPP::SHA256 sha256;
    CryptoPP::StringSource ss(
        data, true,
        new CryptoPP::HashFilter(sha256,
                                 new CryptoPP::HexEncoder(new CryptoPP::StringSink(sha256sum))));
    return sha256sum;
  }

  std::string cal_md5sum(std::string data) {
    string md5sum;
    CryptoPP::Weak1::MD5 md5;
    CryptoPP::StringSource ss(
        data, true,
        new CryptoPP::HashFilter(md5, new CryptoPP::HexEncoder(new CryptoPP::StringSink(md5sum))));

    return md5sum;
  }
}  // namespace spkdfs