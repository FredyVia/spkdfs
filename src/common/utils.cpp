#include "common/utils.h"

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

}  // namespace spkdfs