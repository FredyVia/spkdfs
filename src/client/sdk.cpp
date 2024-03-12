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

  void check_response(const Controller &cntl, const Response &response) {
    if (cntl.Failed()) {
      throw runtime_error("brpc not success: " + cntl.ErrorText());
    }
    if (!response.success()) {
      throw MessageException(response.fail_info());
    }
  }

  SDK::SDK(const std::string &datanode) {
    Controller cntl;
    Request request;
    DNGetNamenodesResponse dn_getnamenodes_resp;
    get_dn_stub(datanode)->get_namenodes(&cntl, &request, &dn_getnamenodes_resp, NULL);
    check_response(cntl, dn_getnamenodes_resp.common());
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

    check_response(cntl, nn_getmaster_resp.common());
    cout << "namenode master: " << nn_getmaster_resp.node() << endl;
    string namenode_master = nn_getmaster_resp.node();

    if (nn_master_channel.Init(string(namenode_master).c_str(), NULL) != 0) {
      throw runtime_error("namenode_master channel init failed");
    }
    nn_master_stub_ptr = new NamenodeService_Stub(&nn_master_channel);
    cout << endl;
  }

  DatanodeService_Stub *SDK::get_dn_stub(const std::string &node) {
    if (dn_stubs.find(node) == dn_stubs.end()) {
      dn_stubs[node].first.Init(node.c_str(), NULL);
      try {
        dn_stubs[node].second = new DatanodeService_Stub(&(dn_stubs[node].first));
      } catch (const exception &e) {
        dn_stubs[node].second = nullptr;
      }
    }
    if (dn_stubs[node].second == nullptr) {
      throw runtime_error("node not online");
    }
    return dn_stubs[node].second;
  }

  SDK::~SDK() {
    if (nn_master_stub_ptr != nullptr) delete nn_master_stub_ptr;
    for (auto &p : dn_stubs) {
      if (p.second.second != nullptr) {
        delete p.second.second;
        p.second.second = nullptr;
      }
    }
  }

  Inode SDK::get_inode(const std::string &dst) {
    Controller cntl;
    NNPathRequest request;
    NNLsResponse response;
    *(request.mutable_path()) = dst;
    nn_master_stub_ptr->ls(&cntl, &request, &response, NULL);
    check_response(cntl, response.common());

    auto _json = nlohmann::json::parse(response.data());
    Inode inode = _json.get<Inode>();

    return inode;
  }

  Inode SDK::ls(const std::string &path) { return get_inode(path); }

  void SDK::mkdir(const string &dst) {
    Controller cntl;
    NNPathRequest request;
    CommonResponse response;
    *(request.mutable_path()) = dst;
    nn_master_stub_ptr->mkdir(&cntl, &request, &response, NULL);
    check_response(cntl, response.common());

    cout << "success" << endl;
  }

  inline std::string SDK::guess_storage_type() const { return "RS<3,2,16>"; }

  std::vector<std::string> SDK::get_datanodes() {
    vector<string> res;
    brpc::Controller cntl;
    Request request;
    DNGetDatanodesResponse response;
    dn_stubs.begin()->second.second->get_datanodes(&cntl, &request, &response, NULL);
    check_response(cntl, response.common());
    for (const auto &node_str : response.nodes()) {
      res.push_back(node_str);
    }
    return res;
  }
  std::vector<std::string> SDK::get_online_datanodes() {
    vector<Node> vec;
    for (const auto &node_str : get_datanodes()) {
      Node node;
      from_string(node_str, node);
      vec.push_back(node);
    }
    node_discovery(vec);
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
                       [](const Node &node) { return node.nodeStatus != NodeStatus::ONLINE; }),
        vec.end());
    vector<string> res;
    for (const auto &node : vec) {
      res.push_back(to_string(node));
    }
    return res;
  }

  void SDK::open(const std::string &path, bool readonly) {
    createDirectoryIfNotExist(get_tmp_path(path));
    if (!readonly) {
      createDirectoryIfNotExist(get_tmp_write_path(path));
    }
  }

  void SDK::put(const string &srcFilePath, const string &dstFilePath,
                const std::string &storage_type) {
    shared_ptr<StorageType> storage_type_ptr = StorageType::from_string(storage_type);

    cout << "opening file " << srcFilePath << endl;
    ifstream file(srcFilePath, ios::binary);
    if (!file.is_open()) {
      throw runtime_error("Failed to open file");
    }
    auto fileSize = fs::file_size(srcFilePath);
    cout << "fileSize: " << fileSize << endl;
    brpc::Controller cntl;
    NNPutRequest nn_put_req;
    NNPutResponse nn_put_resp;
    *(nn_put_req.mutable_path()) = dstFilePath;
    nn_put_req.set_filesize(fileSize);
    *(nn_put_req.mutable_storage_type()) = storage_type;
    nn_master_stub_ptr->put(&cntl, &nn_put_req, &nn_put_resp, NULL);
    check_response(cntl, nn_put_resp.common());

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
      string res = encode_one(storage_type_ptr, block);
      nn_putok_req.add_sub(res);
      blockIndex++;
    }
    CommonResponse response;
    cntl.Reset();
    nn_master_stub_ptr->put_ok(&cntl, &nn_putok_req, &response, NULL);
    check_response(cntl, response.common());

    cout << "put ok" << endl;
  }

  std::string SDK::get_from_datanode(const std::string &datanode, const std::string &blkid) {
    DNGetRequest dnget_req;
    DNGetResponse dnget_resp;
    *(dnget_req.mutable_blkid()) = blkid;
    brpc::Controller cntl;
    get_dn_stub(datanode)->get(&cntl, &dnget_req, &dnget_resp, NULL);
    check_response(cntl, dnget_resp.common());
    cout << datanode << " | " << blkid << " success" << endl;
    string data = dnget_resp.data();
    if (cal_sha256sum(data) != blkid) {
      throw runtime_error("sha256sum check fail");
    };
    return data;
  }

  std::string SDK::put_to_datanode(const string &datanode, const std::string &block) {
    string sha256sum = cal_sha256sum(block);
    cout << "sha256sum:" << sha256sum << endl;
    // 假设每个分块的大小是block.size() / k
    Controller cntl;
    DNPutRequest dn_req;
    CommonResponse dn_resp;
    *(dn_req.mutable_blkid()) = sha256sum;
    *(dn_req.mutable_data()) = block;

    get_dn_stub(datanode)->put(&cntl, &dn_req, &dn_resp, NULL);
    check_response(cntl, dn_resp.common());
    return sha256sum;
  }

  std::string SDK::encode_one(std::shared_ptr<StorageType> storage_type_ptr,
                              const std::string &block) {
    auto vec = storage_type_ptr->encode(block);
    vector<string> nodes = get_online_datanodes();
    string res;
    int node_index = 0, succ = 0;
    Controller cntl;
    // 上传每个编码后的分块
    for (auto &encodedData : vec) {
      cout << "node[" << node_index << "]"
           << ":" << nodes[node_index] << endl;
      string sha256sum = put_to_datanode(nodes[node_index], encodedData);

      std::ostringstream oss;
      oss << std::setw(16) << std::setfill('0') << succ;
      cout << oss.str() << endl;
      res += oss.str() + "|" + to_string(nodes[node_index]) + "|" + sha256sum + ",";

      succ++;
      node_index = (node_index + 1) % nodes.size();
      if (node_index == 0 && storage_type_ptr->check(succ) == false) {
        cout << "may not be secure" << endl;
      }
    }
    res.pop_back();
    return res;
  }

  std::string SDK::decode_one(std::shared_ptr<StorageType> storage_type_ptr, const std::string &s) {
    int succ = 0;
    vector<string> datas;
    datas.resize(storage_type_ptr->getDecodeBlocks());
    stringstream ss(s);
    string str;
    while (getline(ss, str, ',')) {
      stringstream ss(str);
      string _, node, blkid;
      getline(ss, _, '|');      // 提取第一个部分
      getline(ss, node, '|');   // 提取第二个部分
      getline(ss, blkid, '|');  // 提取第三个部分
      cout << _ << "|" << node << "|" << blkid << endl;
      try {
        datas[succ++] = get_from_datanode(node, blkid);
        if (succ == storage_type_ptr->getDecodeBlocks()) {
          return storage_type_ptr->decode(datas);
        }
      } catch (const exception &e) {
        cout << node << " | " << blkid << " failed" << endl;
      }
    }
    return "";
  }

  void SDK::get(const std::string &src, const std::string &dst) {
    Inode inode = get_inode(src);

    cout << "using storage type: " << inode.storage_type_ptr->to_string() << endl;
    ofstream dstFile(dst, std::ios::binary);
    if (!dstFile.is_open()) {
      throw runtime_error("Failed to open file");
    }
    for (const auto &s : inode.sub) {
      dstFile << decode_one(inode.storage_type_ptr, s);
    };
    dstFile.close();
    fs::resize_file(dst, inode.filesize);
  }

  void SDK::rm(const std::string &dst) {
    Controller cntl;
    NNPathRequest request;
    CommonResponse response;
    *(request.mutable_path()) = dst;
    nn_master_stub_ptr->rm(&cntl, &request, &response, NULL);
    check_response(cntl, response.common());

    cout << "success" << endl;
  }

  inline pair<int, int> SDK::get_indexs(const Inode &inode, uint32_t offset, uint32_t size) const {
    return make_pair(align_index_down(offset, inode.getBlockSize()),
                     align_index_up(offset + size, inode.getBlockSize()));
  }

  void SDK::ln_path_index(const std::string &path, uint32_t index) const {
    std::filesystem::create_hard_link(get_tmp_index_path(path, index),
                                      get_ln_path_index(path, index));
  }

  inline std::string SDK::get_ln_path_index(const std::string &path, uint32_t index) const {
    return get_tmp_write_path(path) + "/" + std::to_string(index);
  }

  inline std::string SDK::get_tmp_write_path(const string &path) const {
    return get_tmp_path(path) + "/write";
  }

  inline std::string SDK::get_tmp_path(const string &path) const {
    return "/tmp/spkdfs/fuse/" + cal_md5sum(path);
  }

  std::string SDK::get_tmp_index_path(const string &path, uint32_t index) const {
    string dst_path = get_tmp_path(path);
    return dst_path + "/" + std::to_string(index);
  }

  std::string SDK::read_data(const Inode &inode, std::pair<int, int> indexs) {
    string tmp_path;
    cout << "using storage type: " << inode.storage_type_ptr->to_string() << endl;
    auto iter = inode.sub.begin();
    for (int i = 0; i < indexs.first; i++) {
      iter++;
      if (iter == inode.sub.end()) {
        return "";
      }
    }
    std::ostringstream oss;
    string tmp_str;
    for (int index = indexs.first; index < indexs.second; index++) {
      string blks = *iter;
      tmp_path = get_tmp_index_path(inode.fullpath, index);
      pathlocks.lock(tmp_path);
      if (fs::exists(tmp_path) && fs::file_size(tmp_path) > 0) {
        int filesize = fs::file_size(tmp_path);
        tmp_str.resize(filesize);
        ifstream dstFile(tmp_path, std::ios::binary);
        if (!dstFile) {
          cout << "Failed to open file for reading." << endl;
          throw std::runtime_error("openfile error:" + tmp_path);
        }
        if (!dstFile.read(&tmp_str[0], filesize)) {
          cout << "Failed to read file content." << endl;
          throw std::runtime_error("readfile error:" + tmp_path);
        }
        dstFile.close();

      } else {
        ofstream dstFile(tmp_path, std::ios::binary);
        if (!dstFile) {
          cout << "Failed to open file for reading." << endl;
          throw std::runtime_error("openfile error:" + tmp_path);
        }
        tmp_str = decode_one(inode.storage_type_ptr, blks);
        dstFile << tmp_str;
        dstFile.close();
      }
      pathlocks.unlock(tmp_path);
      oss << tmp_str;
      iter++;
    };
    return oss.str();
  }

  std::string SDK::read_data(const string &path, uint32_t offset, uint32_t size) {
    string res;
    Inode inode = get_inode(path);
    if (inode.filesize < offset) {
      cout << "out of range: filesize: " << inode.filesize << ", offset: " << offset
           << ", size: " << size << endl;
      return "";
    }
    if (inode.filesize < offset + size) {
      size = inode.filesize - offset;
    }
    pair<int, int> indexs = get_indexs(inode, offset, size);
    auto s = read_data(inode, indexs);
    return string(s.begin() + offset - indexs.first * inode.getBlockSize(),
                  s.begin() + offset - indexs.first * inode.getBlockSize() + size);
  }

  void SDK::write_data(const string &path, uint32_t offset, const std::string &s) {
    Inode inode = get_inode(path);

    int size = s.size();
    auto iter = s.begin();

    pair<int, int> indexs = get_indexs(inode, offset, s.size());
    read_data(inode, make_pair(indexs.first, indexs.first + 1));
    read_data(inode, make_pair(indexs.second - 1, indexs.second));

    for (int index = indexs.first; index < indexs.second; index++) {
      size_t left = index == indexs.first ? offset % inode.getBlockSize() : 0;
      size_t right = index == indexs.second - 1 ? (offset + size) % inode.getBlockSize()
                                                : inode.getBlockSize();
      size_t localSize = right - left;
      iter += localSize;

      string dst = get_tmp_index_path(path, index);

      ofstream dstFile(dst, std::ios::binary);
      if (!dstFile) {
        std::cout << "failed to write data to " << dst << std::endl;
        throw runtime_error("failed to write data to " + dst);
      }
      dstFile << string(iter, iter + localSize);
      dstFile.close();
      ln_path_index(path, index);
    }
  }

  void SDK::create(const string &path) {
    string dst_path = get_tmp_index_path(path, 0);
    std::ofstream{dst_path};
    put(dst_path, path, guess_storage_type());
  }

  // std::vector<std::string> SDK::write_data(const Inode &inode, int start_index,
  //                                          const std::string &s) {
  //   vector<string> res;
  //   for (int i = 0; i < s.size(); i += inode.storage_type_ptr->getBlockSize()) {
  //     res.push_back(encode_one(inode.storage_type_ptr, string()));
  //   }
  //   return res;
  // }
  // std::string SDK::get_one_data(const Inode &inode, int index) {
  //   string res;
  //   cout << "using storage type: " << inode.storage_type_ptr->to_string() << endl;
  //   int succ = 0;
  //   auto iter = inode.sub.begin();
  //   for (int i = 0; i < index * inode.storage_type_ptr->getBlocks(); i++) {
  //     iter++;
  //   }
  //   //  for (int index = offset / inode.getBlockSize(); index < blkids.size();
  //   //              index += inode.storage_type_ptr->getBlocks()) {
  //   return decode_one(inode.storage_type_ptr, iter, inode.sub.end());
  // }
}  // namespace spkdfs