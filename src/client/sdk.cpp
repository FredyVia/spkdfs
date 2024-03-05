#include "client/sdk.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "common/exception.h"
#include "common/utils.h"

namespace spkdfs {
  using namespace std;
  using namespace brpc;
  namespace fs = std::filesystem;

  template <typename ResponseType>
  void check_response(const Controller &cntl, const ResponseType &response) {
    if (cntl.Failed()) {
      throw runtime_error("brpc not success: " + cntl.ErrorText());
    }
    if (!response.common().success()) {
      throw MessageException(response.common().fail_info());
    }
  }

  SDK::SDK(const std::string &datanode) {
    if (dn_channel.Init(string(datanode).c_str(), NULL) != 0) {
      throw runtime_error("datanode channel init failed");
    }
    dn_stub_ptr = new DatanodeService_Stub(&dn_channel);

    Controller cntl;
    Request request;
    DNGetNamenodesResponse dn_getnamenodes_resp;
    dn_stub_ptr->get_namenodes(&cntl, &request, &dn_getnamenodes_resp, NULL);
    check_response(cntl, dn_getnamenodes_resp);
    cout << "namenodes: ";
    for (auto node : dn_getnamenodes_resp.nodes()) {
      cout << node << ", ";
    }
    cout << endl;
    string namenode = dn_getnamenodes_resp.nodes(0);
    Channel nn_channel;
    if (nn_channel.Init(string(namenode).c_str(), NULL) != 0) {
      throw runtime_error("get_master channel init failed");
    }

    NamenodeService_Stub nn_stub(&nn_channel);
    NNGetMasterResponse nn_getmaster_resp;
    cntl.Reset();
    nn_stub.get_master(&cntl, &request, &nn_getmaster_resp, NULL);

    check_response(cntl, nn_getmaster_resp);
    cout << "namenode master: " << nn_getmaster_resp.node() << endl;
    string namenode_master = nn_getmaster_resp.node();

    if (nn_master_channel.Init(string(namenode_master).c_str(), NULL) != 0) {
      throw runtime_error("namenode_master channel init failed");
    }
    nn_master_stub_ptr = new NamenodeService_Stub(&nn_master_channel);
    cout << endl;
  }
  SDK::~SDK() {
    if (nn_master_stub_ptr != nullptr) delete nn_master_stub_ptr;
    if (dn_stub_ptr != nullptr) delete dn_stub_ptr;
  }

  Inode SDK::ls(const std::string &dst) {
    Controller cntl;
    NNPathRequest request;
    NNLsResponse response;
    *(request.mutable_path()) = dst;
    nn_master_stub_ptr->ls(&cntl, &request, &response, NULL);
    check_response(cntl, response);

    auto _json = nlohmann::json::parse(response.data());
    Inode inode = _json.get<Inode>();

    return inode;
  }

  void SDK::mkdir(const string &dst) {
    Controller cntl;
    NNPathRequest request;
    CommonResponse response;
    *(request.mutable_path()) = dst;
    nn_master_stub_ptr->mkdir(&cntl, &request, &response, NULL);
    check_response(cntl, response);

    cout << "success" << endl;
  }

  vector<Node> SDK::get_datanodes() {
    brpc::Controller cntl;
    Request request;
    DNGetDatanodesResponse response;
    dn_stub_ptr->get_datanodes(&cntl, &request, &response, NULL);
    check_response(cntl, response);
    vector<Node> vec;
    for (const auto &node_str : response.nodes()) {
      Node node;
      from_string(node_str, node);
      vec.push_back(node);
    }
    node_discovery(vec);
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
                       [](const Node &node) { return node.nodeStatus != NodeStatus::ONLINE; }),
        vec.end());
    return vec;
  }

  void SDK::put(const string &srcFilePath, const string &dstFilePath,
                const std::string &storage_type) {
    auto nodes = get_datanodes();

    vector<Channel> dn_channels(nodes.size());
    for (int i = 0; i < nodes.size(); i++) {
      dn_channels[i].Init(to_string(nodes[i]).c_str(), NULL);
    }
    shared_ptr<StorageType> storage_type_ptr = StorageType::from_string(storage_type);

    cout << "opening file " << srcFilePath << endl;
    ifstream file(srcFilePath, ios::binary | ios::ate);
    if (!file.is_open()) {
      throw runtime_error("Failed to open file");
    }

    auto fileSize = file.tellg();
    cout << "fileSize: " << fileSize << endl;
    file.seekg(0, ios::beg);
    brpc::Controller cntl;
    NNPutRequest nn_put_req;
    NNPutResponse nn_put_resp;
    *(nn_put_req.mutable_path()) = dstFilePath;
    nn_put_req.set_filesize(fileSize);
    *(nn_put_req.mutable_storage_type()) = storage_type;
    nn_master_stub_ptr->put(&cntl, &nn_put_req, &nn_put_resp, NULL);
    check_response(cntl, nn_put_resp);

    NNPutOKRequest nn_putok_req;
    *(nn_putok_req.mutable_path()) = dstFilePath;
    uint64_t block_size = storage_type_ptr->getBlockSize();
    cout << "using block size: " << block_size << endl;
    string block;
    block.resize(block_size);
    int blockIndex = 0;

    int node_index = 0;
    int i = 0;  // used to make std::set be ordered
    while (file.read(&block[0], block_size) || file.gcount() > 0) {
      int succ = 0;
      size_t bytesRead = file.gcount();
      // if (bytesRead < block_size) {
      //   block.resize(block_size, '0');  // 使用0填充到新大小
      // }
      auto vec = storage_type_ptr->encode(block);
      // 上传每个编码后的分块
      for (auto &encodedData : vec) {
        auto sha256sum = cal_sha256sum(encodedData);
        cout << "sha256sum:" << sha256sum << endl;
        // 假设每个分块的大小是block.size() / k

        cout << "node[" << node_index << "]"
             << ":" << nodes[node_index] << endl;
        DNPutRequest dn_req;
        CommonResponse dn_resp;
        DatanodeService_Stub dn_stub(&dn_channels[node_index]);
        *(dn_req.mutable_blkid()) = sha256sum;
        *(dn_req.mutable_data()) = encodedData;
        cntl.Reset();
        dn_stub.put(&cntl, &dn_req, &dn_resp, NULL);
        check_response(cntl, dn_resp);

        std::ostringstream oss;
        oss << std::setw(16) << std::setfill('0') << i++;
        cout << oss.str() << endl;
        nn_putok_req.add_sub(oss.str() + "|" + to_string(nodes[node_index]) + "|" + sha256sum);

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
    nn_master_stub_ptr->put_ok(&cntl, &nn_putok_req, &response, NULL);
    check_response(cntl, response);

    cout << "put ok" << endl;
  }

  std::string SDK::get_from_datanode(const std::string &datanode, const std::string &blkid) {
    Channel dn_channel;
    dn_channel.Init(datanode.c_str(), NULL);
    DatanodeService_Stub dn_stub(&dn_channel);
    DNGetRequest dnget_req;
    DNGetResponse dnget_resp;
    *(dnget_req.mutable_blkid()) = blkid;
    brpc::Controller cntl;
    dn_stub.get(&cntl, &dnget_req, &dnget_resp, NULL);
    check_response(cntl, dnget_resp);
    cout << datanode << " | " << blkid << " success" << endl;
    string data = dnget_resp.data();
    cout << "dnget_resp blk size: " << data.size() << endl;
    assert(cal_sha256sum(data) == blkid);
    return data;
  }

  void SDK::get(const string &src, const string &dst) {
    brpc::Controller cntl;
    NNPathRequest request;
    NNGetResponse nnget_resp;
    *(request.mutable_path()) = src;
    nn_master_stub_ptr->get(&cntl, &request, &nnget_resp, NULL);
    check_response(cntl, nnget_resp);
    cout << "using storage type: " << nnget_resp.storage_type() << endl;
    uint64_t filesize = nnget_resp.filesize();
    auto storage_type_ptr = StorageType::from_string(nnget_resp.storage_type());
    ofstream dstFile(dst, std::ios::out);
    if (!dstFile.is_open()) {
      throw runtime_error("Failed to open file");
    }

    vector<string> datas;
    datas.resize(storage_type_ptr->getDecodeBlocks());
    int succ = 0;
    for (int index = 0; index < nnget_resp.blkids_size(); index++) {
      if (succ == storage_type_ptr->getDecodeBlocks()) {
        auto str = storage_type_ptr->decode(datas);
        // str.resize(storage_type_ptr);
        dstFile << str;
        while (index % storage_type_ptr->getBlocks() != 0) index++;
        if (index >= nnget_resp.blkids_size()) {
          break;
        }
        succ = 0;
      }
      string str = nnget_resp.blkids(index);
      stringstream ss(str);
      string _, node, blkid;
      getline(ss, _, '|');      // 提取第一个部分
      getline(ss, node, '|');   // 提取第二个部分
      getline(ss, blkid, '|');  // 提取第三个部分
      cout << _ << "|" << node << "|" << blkid << endl;
      try {
        datas[succ++] = get_from_datanode(node, blkid);
      } catch (const exception &e) {
        cout << node << " | " << blkid << " failed" << endl;
      }
    };
    dstFile.close();
    fs::resize_file(dst, filesize);
  }

  void SDK::rm(const std::string &dst) {
    Controller cntl;
    NNPathRequest request;
    CommonResponse response;
    *(request.mutable_path()) = dst;
    nn_master_stub_ptr->rm(&cntl, &request, &response, NULL);
    check_response(cntl, response);

    cout << "success" << endl;
  }

  std::string get_part(const std::string &path, uint32_t offset, uint32_t size) {
    string res;

    return res;
  }

  void put_part(const std::string &path, uint32_t offset, uint32_t size) {}
}  // namespace spkdfs