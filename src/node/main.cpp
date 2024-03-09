// breakpad must be in first line(cause: #error "inttypes.h has already been included before this
// header file, but ")
#if (1)
#  include "client/linux/handler/exception_handler.h"
#endif
#include <glog/logging.h>

#include <iostream>

#include "common/node.h"
#include "common/utils.h"
#include "gflags/gflags.h"
#include "node/config.h"
#include "node/server.h"

using namespace std;
using namespace spkdfs;

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  createDirectoryIfNotExist(FLAGS_log_dir);
  google::InitGoogleLogging(argv[0]);
  createDirectoryIfNotExist(FLAGS_coredumps_dir);
  google_breakpad::MinidumpDescriptor descriptor(FLAGS_coredumps_dir);
  createDirectoryIfNotExist(FLAGS_data_dir);
  auto nodes = parse_nodes(FLAGS_nodes);
  spkdfs::Server server(nodes);
  LOG(INFO) << "going to start server";
  server.start();
  // cout << "exit" ;
  return 0;
}