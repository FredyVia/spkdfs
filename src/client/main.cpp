#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller

#include <iostream>
#include <string>
#include <vector>

#include "common/node.h"
#include "service.pb.h"
using namespace std;
using namespace spkdfs;
using namespace brpc;

DEFINE_string(datanode, "127.0.0.1", "skip when namenode or master specified");
DEFINE_string(namenode, "", "skip when master avaiable");
DEFINE_string(namenode_master, "", "optional, namenode_master addr: <addr>");

NamenodeService_Stub* nn_master_stub;
Channel nn_master_channel;
void Init() {
  if (FLAGS_namenode_master == "" && FLAGS_namenode == "") {
    Channel dn_channel;
    if (dn_channel.Init(string(FLAGS_datanode).c_str(), NULL) != 0) {
      throw runtime_error("channel init failed");
    }
    DatanodeService_Stub dn_stub(&dn_channel);

    Controller cntl;
    Request request;
    DNGetNamenodesResponse response;
    dn_stub.get_namenodes(&cntl, &request, &response, NULL);

    if (cntl.Failed()) {
      LOG(ERROR) << "RPC failed: " << cntl.ErrorText() << std::endl;
      throw runtime_error("rpc get_namenodes failed");
    }
    if (!response.common().success()) {
      throw runtime_error("response show not success");
    }
    gflags::SetCommandLineOption("namenode", response.nodes(0).c_str());
  }
  if (FLAGS_namenode_master == "") {
    Channel nn_channel;
    if (nn_channel.Init(string(FLAGS_namenode).c_str(), NULL) != 0) {
      throw runtime_error("channel init failed");
    }

    NamenodeService_Stub nn_stub(&nn_channel);

    Controller cntl;
    Request request;
    NNGetMasterResponse response;
    nn_stub.get_master(&cntl, &request, &response, NULL);

    if (cntl.Failed()) {
      LOG(ERROR) << "RPC failed: " << cntl.ErrorText() << std::endl;
      throw runtime_error("rpc get_namenodes failed");
    }
    if (!response.common().success()) {
      throw runtime_error("response show not success");
    }
    gflags::SetCommandLineOption("namenode_master", response.node().c_str());
  }

  if (nn_master_channel.Init(string(FLAGS_namenode_master).c_str(), NULL) != 0) {
    throw runtime_error("channel init failed");
  }
  nn_master_stub = new NamenodeService_Stub(&nn_master_channel);
}
void mkdir() {}

vector<Node> get_namenodes() {}

int main(int argc, char* argv[]) {
  // 初始化 channel
  Init();
  std::string command = argv[1];
  if (command == "-put") {
    // 处理 -put 命令
  } else if (command == "-get") {
    // 处理 -get 命令
  } else if (command == "-ls") {
    // 处理 -ls 命令
  } else if (command == "-mkdir") {
    // 处理 -mkdir 命令
  } else {
  }

  return 0;
}