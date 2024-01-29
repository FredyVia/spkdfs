
#include <glog/logging.h>

#include <iostream>

#include "common/config.h"
#include "common/node.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "node/server.h"

using namespace std;
using namespace spkdfs;
bool createDirectoryIfNotExist(const std::string& dir) {
  struct stat info;
  if (stat(dir.c_str(), &info) != 0) {
    // 目录不存在，尝试创建
    if (mkdir(dir.c_str(), 0755) == -1) {
      std::cerr << "Error: Unable to create directory " << dir << std::endl;
      return false;
    }
  } else if (!(info.st_mode & S_IFDIR)) {
    // 路径存在，但不是一个目录
    std::cerr << "Error: " << dir << " exists and is not a directory." << std::endl;
    return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  try {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    createDirectoryIfNotExist(FLAGS_log_dir);
    google::InitGoogleLogging(argv[0]);
    auto nodes = parse_nodes(FLAGS_nodes);
    spkdfs::Server server(nodes);
    server.start();
    // cout << "exit" << endl;
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
  }
  return 0;
}