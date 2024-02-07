#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "client/mln_rs.h"
#include "client/mln_sha.h"
#include "common/inode.h"
#include "common/node.h"
#include "service.pb.h"
#define ROUND_UP_8(num) ((num + 7) & ~7)
using namespace std;
using namespace spkdfs;
using namespace brpc;

DEFINE_string(command, "ls", "command type: ls mkdir put get");
DEFINE_string(datanode, "127.0.0.1:18001", "required");
DEFINE_string(namenode, "", "optional");
DEFINE_string(namenode_master, "", "optional, namenode_master addr: <addr>:<port>");
DEFINE_uint32(block_size, 64 * 1024, "optional, BLOCK size Bytes");
template <typename ResponseType>
void check_response(const Controller &cntl, const ResponseType &response) {
  if (cntl.Failed()) {
    throw std::runtime_error("brpc not success: " + cntl.ErrorText());
  }
  if (!response.common().success()) {
    throw std::runtime_error(response.common().fail_info());
  }
}
NamenodeService_Stub *nn_master_stub;
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
      throw runtime_error(response.common().fail_info());
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
      throw runtime_error(response.common().fail_info());
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
void ls(const string &s) {
  Controller cntl;
  NNPathRequest request;
  NNLsResponse response;
  *(request.mutable_path()) = s;
  nn_master_stub->ls(&cntl, &request, &response, NULL);
  if (cntl.Failed()) {
    cerr << "RPC failed: " << cntl.ErrorText();
    throw runtime_error("RPC failed: " + cntl.ErrorText());
  }
  if (!response.common().success()) {
    cerr << "failed" << response.common().fail_info() << endl;
    throw runtime_error(response.common().fail_info());
  }
  for (auto str : response.data()) {
    cout << str << endl;
  }
}
void mkdir(const string &dst) {
  Controller cntl;
  NNPathRequest request;
  CommonResponse response;
  *(request.mutable_path()) = dst;
  nn_master_stub->mkdir(&cntl, &request, &response, NULL);
  if (cntl.Failed()) {
    cerr << "RPC failed: " << cntl.ErrorText();
    throw runtime_error("RPC failed: " + cntl.ErrorText());
  }
  if (!response.common().success()) {
    cerr << "failed" << response.common().fail_info() << endl;
    throw runtime_error(response.common().fail_info());
  }
  cout << "success" << endl;
}

void get_k_m(int &k, int &m) {
  int all = 12;
  k = 3;
  m = 2;
}

vector<Node> get_datanodes() {
  Channel dn_channel;
  if (dn_channel.Init(string(FLAGS_datanode).c_str(), NULL) != 0) {
    throw runtime_error("datanode channel init failed");
  }
  DatanodeService_Stub dn_stub(&dn_channel);

  brpc::Controller cntl;
  Request request;
  DNGetDatanodesResponse response;
  dn_stub.get_datanodes(&cntl, &request, &response, NULL);
  if (cntl.Failed()) {
    throw runtime_error("brpc not success");
  }
  if (!response.common().success()) {
    throw runtime_error(response.common().fail_info());
  }
  vector<Node> vec;
  for (auto node_str : response.nodes()) {
    Node node;
    from_string(node_str, node);
    vec.push_back(node);
  }
  return vec;
}

void put(const string &srcFilePath, const string &dstFilePath) {
  auto nodes = get_datanodes();
  int k = 0, m = 0;
  get_k_m(k, m);
  cout << "using k,m=" << k << "," << m << endl;
  mln_sha256_t s;
  mln_sha256_init(&s);
  cout << "using block size: " << FLAGS_block_size << endl;
  string block;
  block.reserve(FLAGS_block_size);

  cout << "opening file " << srcFilePath << endl;
  ifstream file(srcFilePath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    std::cerr << "Failed to open file\n";
    return;
  }

  auto fileSize = file.tellg();
  cout << "fileSize: " << fileSize << endl;
  file.seekg(0, std::ios::beg);
  brpc::Controller cntl;
  NNPutRequest request;
  NNPutResponse nnput_resp;
  *(request.mutable_path()) = dstFilePath;
  request.set_filesize(fileSize);
  *(request.mutable_storage_type()) = to_string(StorageType::STORAGETYPE_RS);
  nn_master_stub->put(&cntl, &request, &nnput_resp, NULL);
  if (cntl.Failed()) {
    throw runtime_error("brpc not success");
  }
  if (!nnput_resp.common().success()) {
    cerr << "error" << endl;
    throw runtime_error(nnput_resp.common().fail_info());
  }
  block.resize(FLAGS_block_size);
  int blockIndex = 0;
  while (file.read(&block[0], FLAGS_block_size) || file.gcount() > 0) {
    int succ = 0;
    size_t bytesRead = file.gcount();
    if (bytesRead < FLAGS_block_size) {
      // size_t newSize = ROUND_UP_8(bytesRead);
      block.resize(FLAGS_block_size, 0);  // 使用0填充到新大小
    }

    // 使用Melon库进行纠删码编码
    mln_rs_result_t *res = mln_rs_encode((uint8_t *)block.data(), block.size() / k, k, m);
    if (res == nullptr) {
      std::cerr << "rs encode failed.\n";
      break;
    }

    // 上传每个编码后的分块
    for (int j = 0; j < k + m; j++) {
      uint8_t *encodedData = mln_rs_result_get_data_by_index(res, j);
      if (encodedData != nullptr) {
        // 假设每个分块的大小是block.size() / k

        char sha256sum[1024] = {0};
        mln_sha256_calc(&s, encodedData, block.size() / k, 1);
        mln_sha256_tostring(&s, sha256sum, sizeof(sha256sum) - 1);
        cout << "sha256sum:" << sha256sum << endl;
        Channel dn_channel;
        cout << "node[" << j << "]"
             << ":" << nodes[j] << endl;
        if (dn_channel.Init(to_string(nodes[j]).c_str(), NULL) != 0) {
          throw runtime_error("datanode channel init failed");
        }
        DNPutRequest dn_req;
        CommonResponse dn_resp;
        DatanodeService_Stub dn_stub(&dn_channel);

        *(dn_req.mutable_blkid()) = string(sha256sum);
        *(dn_req.mutable_data()) = string((char *)encodedData);
        cntl.Reset();
        dn_stub.put(&cntl, &dn_req, &dn_resp, NULL);
        check_response(cntl, dn_resp);
        succ++;
      }
    }
    mln_rs_result_free(res);
    if (succ < k) {
      throw runtime_error("succ < k");
    }

    blockIndex++;
  }
  NNPutOKRequest nnputok_req;
  CommonResponse response;
  *(nnputok_req.mutable_path()) = dstFilePath;
  cntl.Reset();
  nn_master_stub->put_ok(&cntl, &nnputok_req, &response, NULL);
  if (cntl.Failed()) {
    throw runtime_error("brpc not success");
  }
  if (!response.common().success()) {
    throw runtime_error(response.common().fail_info());
  }
  cout << "put ok" << endl;
}

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  // if (argc < 2) {
  //   cout << "help" << endl;
  //   return -1;
  // }
  // 初始化 channel
  Init();
  for (int i = 1; i < argc; ++i) {
    std::cout << "Remaining arg: " << argv[i] << std::endl;
  }
  if (FLAGS_command == "put") {
    put(argv[1], argv[2]);
    // 处理 put 命令
  } else if (FLAGS_command == "get") {
    // 处理 get 命令
  } else if (FLAGS_command == "ls") {
    ls(argv[1]);
  } else if (FLAGS_command == "mkdir") {
    mkdir(argv[1]);
  } else {
    cerr << "unknown command" << endl;
  }
  return 0;
}