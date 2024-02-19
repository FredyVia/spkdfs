#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "common/inode.h"
#include "common/mln_rs.h"
#include "common/mln_sha.h"
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
DEFINE_string(storage_type, "rs(3,2)", "rs or replication, example: rs(3,2) re");
template <typename ResponseType>
void check_response(const Controller &cntl, const ResponseType &response) {
  if (cntl.Failed()) {
    throw runtime_error("brpc not success: " + cntl.ErrorText());
  }
  if (!response.common().success()) {
    throw runtime_error(response.common().fail_info());
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
    cout << "namenode: " << response.nodes(0) << endl;
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
    cout << "namenode master: " << response.node() << endl;
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
  shared_ptr<StorageType> storage_type_ptr = StorageType::from_string(string(FLAGS_storage_type));
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
  check_response(cntl, response);
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

  vector<Channel> dn_channels(nodes.size());
  for (int i = 0; i < nodes.size(); i++) {
    dn_channels[i].Init(to_string(nodes[i]).c_str(), NULL);
  }
  shared_ptr<StorageType> storage_type_ptr = StorageType::from_string(FLAGS_storage_type);
  mln_sha256_t s;
  mln_sha256_init(&s);

  cout << "opening file " << srcFilePath << endl;
  ifstream file(srcFilePath, ios::binary | ios::ate);
  if (!file.is_open()) {
    cerr << "Failed to open file\n";
    return;
  }

  auto fileSize = file.tellg();
  cout << "fileSize: " << fileSize << endl;
  file.seekg(0, ios::beg);
  brpc::Controller cntl;
  NNPutRequest request;
  NNPutResponse nnput_resp;
  *(request.mutable_path()) = dstFilePath;
  request.set_filesize(fileSize);
  *(request.mutable_storage_type()) = FLAGS_storage_type;
  nn_master_stub->put(&cntl, &request, &nnput_resp, NULL);
  check_response(cntl, nnput_resp);

  NNPutOKRequest nnputok_req;
  *(nnputok_req.mutable_path()) = dstFilePath;

  cout << "using block size: " << FLAGS_block_size << endl;
  string block;
  block.resize(FLAGS_block_size);
  int blockIndex = 0;
  char sha256sum[1024] = {0};
  int node_index = 0;
  int i = 0;  // used to make std::set be ordered
  while (file.read(&block[0], FLAGS_block_size) || file.gcount() > 0) {
    int succ = 0;
    size_t bytesRead = file.gcount();
    if (bytesRead < FLAGS_block_size) {
      // size_t newSize = ROUND_UP_8(bytesRead);
      block.resize(FLAGS_block_size, 0);  // 使用0填充到新大小
    }
    auto vec = storage_type_ptr->encode(block);
    // 上传每个编码后的分块
    for (auto &encodedData : vec) {
      mln_sha256_calc(&s, (mln_u8ptr_t)encodedData.c_str(), encodedData.size(), 1);
      mln_sha256_tostring(&s, sha256sum, sizeof(sha256sum) - 1);
      cout << "sha256sum:" << sha256sum << endl;
      // 假设每个分块的大小是block.size() / k

      cout << "node[" << node_index << "]"
           << ":" << nodes[node_index] << endl;
      DNPutRequest dn_req;
      CommonResponse dn_resp;
      DatanodeService_Stub dn_stub(&dn_channels[node_index]);
      *(dn_req.mutable_blkid()) = string(sha256sum);
      *(dn_req.mutable_data()) = encodedData;
      cntl.Reset();
      dn_stub.put(&cntl, &dn_req, &dn_resp, NULL);
      check_response(cntl, dn_resp);

      std::ostringstream oss;
      oss << std::setw(16) << std::setfill('0') << i++;
      cout << oss.str() << endl;
      nnputok_req.add_sub(oss.str() + "|" + to_string(nodes[node_index]) + "|" + sha256sum);

      succ++;
      node_index = (node_index + 1) % nodes.size();
      if (node_index == 0 && storage_type_ptr->check(succ) == false) {
        cout << "may not be secure" << endl;
      }
    }
    blockIndex++;
  }
  CommonResponse response;
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

void get(const string &src, const string &dst) {
  brpc::Controller cntl;
  NNPathRequest request;
  NNGetResponse nnget_resp;
  *(request.mutable_path()) = src;
  nn_master_stub->get(&cntl, &request, &nnget_resp, NULL);
  check_response(cntl, nnget_resp);
  cout << "using storage type: " << nnget_resp.storage_type() << endl;
  auto storage_type_ptr = StorageType::from_string(nnget_resp.storage_type());
  for (auto &str : nnget_resp.blkids()) {
    std::stringstream ss(str);
    std::string _, node, blkid;
    std::getline(ss, _, '|');      // 提取第一个部分
    std::getline(ss, node, '|');   // 提取第二个部分
    std::getline(ss, blkid, '|');  // 提取第三个部分
    cout << _ << "|" << node << "|" << blkid << endl;
    Channel dn_channel;

    dn_channel.Init(node.c_str(), NULL);
    DatanodeService_Stub dn_stub(&dn_channel);
    DNGetRequest dnget_req;
    DNGetResponse dnget_resp;
    *(dnget_req.mutable_blkid()) = blkid;
    cntl.Reset();
    dn_stub.get(&cntl, &dnget_req, &dnget_resp, NULL);
  };
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
    cout << "Remaining arg: " << argv[i] << endl;
  }
  if (FLAGS_command == "put") {
    put(argv[1], argv[2]);
    // 处理 put 命令
  } else if (FLAGS_command == "get") {
    get(argv[1], argv[2]);
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