// breakpad must be in first line(cause: #error "inttypes.h has already been included before this
// header file, but ")
#if (1)
#  include "client/linux/handler/exception_handler.h"
#endif
#include <glog/logging.h>

#include <iostream>

#include "backward.hpp"
#include "common/node.h"
#include "gflags/gflags.h"
#include "node/config.h"
#include "node/server.h"

using namespace std;
using namespace spkdfs;
bool createDirectoryIfNotExist(const std::string& dir) {
  struct stat info;
  if (stat(dir.c_str(), &info) != 0) {
    // 目录不存在，尝试创建
    if (mkdir(dir.c_str(), 0755) == -1) {
      LOG(ERROR) << "Error: Unable to create directory " << dir << std::endl;
      return false;
    }
  } else if (!(info.st_mode & S_IFDIR)) {
    // 路径存在，但不是一个目录
    LOG(ERROR) << "Error: " << dir << " exists and is not a directory." << std::endl;
    return false;
  }
  return true;
}
static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context,
                         bool succeeded) {
  printf("Dump path: %s\n", descriptor.path());
  return succeeded;
}
int main(int argc, char* argv[]) {
  backward::SignalHandling sh;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  createDirectoryIfNotExist(FLAGS_log_dir);
  google::InitGoogleLogging(argv[0]);

  createDirectoryIfNotExist(FLAGS_coredumps_dir);
  google_breakpad::MinidumpDescriptor descriptor(FLAGS_coredumps_dir);
  google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback, NULL, true, -1);
  auto nodes = parse_nodes(FLAGS_nodes);
  spkdfs::Server server(nodes);
  server.start();
  // cout << "exit" << endl;
  return 0;
}