
#include <iostream>
#include <string>
#include <vector>

#include "client/sdk.h"

using namespace std;
using namespace spkdfs;

DEFINE_string(command, "ls", "command type: ls mkdir put get");
DEFINE_string(datanode, "192.168.88.112:8001", "datanode addr:port");
DEFINE_string(storage_type, "RS<3,2,64>",
              "rs or replication, example: rs<3,2,64> re<5,64>, k=3,m=2,blocksize=64MB ");
DEFINE_string(namenode, "", "set the default namenode to read, addr:port");

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  // 将日志信息输出到标准输出
  FLAGS_logtostderr = true;
  SDK sdk(FLAGS_datanode, FLAGS_namenode);

  if (FLAGS_command == "put") {
    sdk.put(argv[1], argv[2], FLAGS_storage_type);
    cout << "success" << endl;
  } else if (FLAGS_command == "get") {
    sdk.get(argv[1], argv[2]);
    cout << "success" << endl;
  } else if (FLAGS_command == "ls") {
    Inode inode = sdk.ls(argv[1]);
    if (inode.is_directory) {
      // cout << '.' << endl;
      // cout << '..' << endl;
      for (auto sub : inode.sub) {
        cout << sub << endl;
      }
    } else {
      cout << inode.get_fullpath() << " " << inode.filesize << " "
           << (inode.storage_type_ptr == nullptr ? "UNKNOWN" : inode.storage_type_ptr->to_string())
           << endl;
    }
  } else if (FLAGS_command == "mkdir") {
    sdk.mkdir(argv[1]);
    cout << "success" << endl;
  } else if (FLAGS_command == "rm") {
    sdk.rm(argv[1]);
    cout << "success" << endl;
  } else {
    LOG(ERROR) << "unknown command";
  }
  return 0;
}