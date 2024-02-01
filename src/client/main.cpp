#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller

#include <iostream>
#include <string>
#include <vector>
#include "mln_rs.h"

#include "common/node.h"
#include "service.pb.h"
using namespace std;
using namespace spkdfs;
using namespace brpc;

DEFINE_string(datanode, "127.0.0.1:18001", "skip when namenode or master specified");
DEFINE_string(namenode, "", "skip when master avaiable");
DEFINE_string(namenode_master, "", "optional, namenode_master addr: <addr>");

NamenodeService_Stub* nn_master_stub;
Channel nn_master_channel;
void Init() {
  if (FLAGS_namenode_master == "" && FLAGS_namenode == "") {
    Channel dn_channel;
    if (dn_channel.Init(string(FLAGS_datanode).c_str(), NULL) != 0) {
      throw runtime_error("datanode channel init failed");
    }
    DatanodeService_Stub dn_stub(&dn_channel);

    Controller cntl;
    Request request;
    DNGetNamenodesResponse response;
    dn_stub.get_namenodes(&cntl, &request, &response, NULL);

    if (cntl.Failed()) {
      cerr << "RPC failed: " << cntl.ErrorText();
      throw runtime_error("rpc get_namenodes failed");
    }
    if (!response.common().success()) {
      throw runtime_error(string("get_namenodes response show not success") + " | "
                          + response.common().fail_info());
    }
    cout << "namenode" << response.nodes(0) << endl;
    gflags::SetCommandLineOption("namenode", response.nodes(0).c_str());
  }
  if (FLAGS_namenode_master == "") {
    Channel nn_channel;
    if (nn_channel.Init(string(FLAGS_namenode).c_str(), NULL) != 0) {
      throw runtime_error("get_master channel init failed");
    }

    NamenodeService_Stub nn_stub(&nn_channel);

    Controller cntl;
    Request request;
    NNGetMasterResponse response;
    nn_stub.get_master(&cntl, &request, &response, NULL);

    if (cntl.Failed()) {
      cerr << "RPC failed: " << cntl.ErrorText();
      throw runtime_error("rpc get_master failed");
    }
    if (!response.common().success()) {
      throw runtime_error(string("get_master response show not success") + " | "
                          + response.common().fail_info());
    }
    cout << "namenode master" << response.node() << endl;
    gflags::SetCommandLineOption("namenode_master", response.node().c_str());
  }

  if (nn_master_channel.Init(string(FLAGS_namenode_master).c_str(), NULL) != 0) {
    throw runtime_error("namenode_master channel init failed");
  }
  nn_master_stub = new NamenodeService_Stub(&nn_master_channel);
  cout << "successfully connected to namenode master" << endl;
}
void mkdir() {}

vector<Node> get_namenodes() {}

int main(int argc, char* argv[]) {
  // 初始化 channel
  if (argc < 3) {
    cout << "help" << endl;
    return -1;
  }
  Init();

  std::string command = argv[1];

  Controller cntl;
  if (command == "-put") {

    // 处理 -put 命令
  } else if (command == "-get") {
    // 处理 -get 命令
  } else if (command == "-ls") {
    NNLsRequest request;
    NNLsResponse response;
    *(request.mutable_path()) = argv[2];
    nn_master_stub->ls(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
      cerr << "RPC failed: " << cntl.ErrorText();
      return -2;
    }
    if (!response.common().success()) {
      cerr << command << " failed" << endl;
      return -2;
    }
    for (auto str : response.data()) {
      cout << str << endl;
    }
    // 处理 -ls 命令
  } else if (command == "-mkdir") {
    NNMkdirRequest request;
    CommonResponse response;
    *(request.mutable_path()) = argv[2];
    nn_master_stub->mkdir(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
      cerr << "RPC failed: " << cntl.ErrorText();
      return -2;
    }
    if (!response.common().success()) {
      cerr << command << " failed" << endl;
      return -2;
    }
    cout << "success" << endl;
  } else {
    cerr << "unknown command" << endl;
  }
  return 0;
}