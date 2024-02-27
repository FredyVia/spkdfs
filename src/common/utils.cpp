#include "common/utils.h"

#include <glog/logging.h>

#include <filesystem>

#include "common/mln_sha.h"
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
    mln_sha256_t s;
    mln_sha256_init(&s);
    char sha256sum[1024] = {0};
    mln_sha256_calc(&s, (mln_u8ptr_t)data.c_str(), data.size(), 1);
    mln_sha256_tostring(&s, sha256sum, sizeof(sha256sum) - 1);
    return sha256sum;
  }

}  // namespace spkdfs