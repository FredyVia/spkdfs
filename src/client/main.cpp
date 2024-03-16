
#include <iostream>
#include <string>
#include <vector>

#include "client/sdk.h"

using namespace std;
using namespace spkdfs;

DEFINE_string(command, "ls", "command type: ls mkdir put get");
DEFINE_string(datanode, "127.0.0.1:18001", "datanode addr:port");
DEFINE_string(storage_type, "RS<3,2,64>",
              "rs or replication, example: rs<3,2,64> re<5,64>, k=3,m=2,blocksize=64MB ");

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  // if (argc < 2) {
  //   cout << "help" << endl;
  //   return -1;
  // }
  // 初始化 channel
  SDK sdk(FLAGS_datanode);
  // for (int i = 1; i < argc; ++i) {
  // cout << "Remaining arg: " << argv[i] << endl;
  // }
  if (FLAGS_command == "put") {
    sdk.put(argv[1], argv[2], FLAGS_storage_type);
  } else if (FLAGS_command == "get") {
    sdk.get(argv[1], argv[2]);
  } else if (FLAGS_command == "ls") {
    Inode inode = sdk.ls(argv[1]);
    if (inode.is_directory) {
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
  } else if (FLAGS_command == "rm") {
    sdk.rm(argv[1]);
  } else {
    cerr << "unknown command" << endl;
  }
  return 0;
}