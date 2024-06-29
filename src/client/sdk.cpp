#include "client/sdk.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "common/config.h"
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

  SDK::SDK(const std::string &datanode, const std::string &namenode) {
    // get all namenodes
    Controller cntl;
    Request request;
    DNGetNamenodesResponse dn_getnamenodes_resp;
    vector<string> namenodes;
    get_dn_stub(datanode)->get_namenodes(&cntl, &request, &dn_getnamenodes_resp, NULL);
    check_response(cntl, dn_getnamenodes_resp.common());
    cout << "namenodes: ";
    for (auto node : dn_getnamenodes_resp.nodes()) {
      namenodes.push_back(node);
      VLOG(2) << node;
    }

    // get the master namenode
    string namenode_slave = namenode;
    if (namenode_slave == "") {
      namenode_slave = get_slave_namenode(namenodes);
    }
    cout << "namenode slave: " << namenode_slave << endl;
    if (nn_slave_channel.Init(string(namenode_slave).c_str(), NULL) != 0) {
      throw runtime_error("namenode_slave channel init failed");
    }
    nn_slave_stub_ptr = new NamenodeService_Stub(&nn_slave_channel);

    NNGetMasterResponse nn_getmaster_resp;
    cntl.Reset();
    nn_slave_stub_ptr->get_master(&cntl, &request, &nn_getmaster_resp, NULL);
    check_response(cntl, nn_getmaster_resp.common());

    string namenode_master = nn_getmaster_resp.node();
    VLOG(2) << "namenode master: " << namenode_master;

    // set the master namenode service
    if (nn_master_channel.Init(string(namenode_master).c_str(), NULL) != 0) {
      throw runtime_error("namenode_master channel init failed");
    }
    nn_master_stub_ptr = new NamenodeService_Stub(&nn_master_channel);

    // set the slave namenode service
    if (namenode_master == namenode_slave) {
      namenodes.erase(remove(namenodes.begin(), namenodes.end(), namenode_slave), namenodes.end());
      namenode_slave = get_slave_namenode(namenodes);
      cout << "namenode slave: " << namenode_slave << endl;
      if (nn_slave_channel.Init(string(namenode_slave).c_str(), NULL) != 0) {
        throw runtime_error("namenode_slave channel init failed");
      }
      delete nn_slave_stub_ptr;
      nn_slave_stub_ptr = new NamenodeService_Stub(&nn_slave_channel);
      cout << endl;
    }

    timer = make_shared<IntervalTimer>(
        max(LOCK_REFRESH_INTERVAL >> 2, 5),
        [this]() {
          if (remoteLocks.empty()) {
            return;
          }
          int retryCount = 0;  // 重试计数器
          while (true) {
            try {
              vector<string> paths;
              {
                std::shared_lock<std::shared_mutex> lock(mutex_remoteLocks);
                paths = vector<string>(remoteLocks.begin(), remoteLocks.end());
              }
              _lock_remote(paths);

              break;
            } catch (const exception &e) {
              if (retryCount >= 3) {  // 允许最多重试3次
                LOG(ERROR) << "retry error" << e.what();
                throw IntervalTimer::TimeoutException("timeout", e.what());
              }
              retryCount++;  // 增加重试计数器
              continue;      // 跳过当前迭代的其余部分
            }
          }
        },
        [this](const void *args) {
          throw runtime_error(string("update lock fail ") + (const char *)args);
        });
  }

  DatanodeService_Stub *SDK::get_dn_stub(const std::string &node) {
    std::lock_guard<std::mutex> lock(mutex_dn_stubs);
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

  string SDK::get_slave_namenode(const vector<string> &namenodes) {
    srand(time(0));
    int index = rand() % namenodes.size();
    return namenodes[index];
  }

  SDK::~SDK() {
    if (nn_master_stub_ptr != nullptr) delete nn_master_stub_ptr;
    if (nn_slave_stub_ptr != nullptr) delete nn_slave_stub_ptr;
    for (auto &p : dn_stubs) {
      if (p.second.second != nullptr) {
        delete p.second.second;
        p.second.second = nullptr;
      }
    }
  }

  Inode SDK::get_inode(const std::string &dst) {
    if (local_inode_cache.find(dst) == local_inode_cache.end()) {
      throw runtime_error("cannot find inode in local_inode_cache");
    }
    return local_inode_cache[dst];
  }

  Inode SDK::get_remote_inode(const std::string &dst) {
    // if(inodeCache.find(dst) != inodeCache.end()){
    //   return inodeCache[dst];
    // }
    Controller cntl;
    NNPathRequest request;
    NNLsResponse response;
    *(request.mutable_path()) = dst;
    nn_slave_stub_ptr->ls(&cntl, &request, &response, NULL);
    check_response(cntl, response.common());

    auto _json = nlohmann::json::parse(response.data());
    Inode inode = _json.get<Inode>();
    VLOG(2) << inode.value();
    return inode;
  }

  Inode SDK::ls(const std::string &path) { return get_remote_inode(path); }

  void SDK::mkdir(const string &dst) {
    Controller cntl;
    NNPathRequest request;
    CommonResponse response;
    *(request.mutable_path()) = dst;
    nn_master_stub_ptr->mkdir(&cntl, &request, &response, NULL);
    check_response(cntl, response.common());
  }

  inline std::string SDK::guess_storage_type() const { return "RS<7,5,16>"; }

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

  void SDK::update_inode(const std::string &path) {
    std::shared_lock<std::shared_mutex> local_read_lock(mutex_local_inode_cache);
    Inode local = local_inode_cache[path];
    local_read_lock.unlock();

    Inode remote = get_remote_inode(path);
    for (int i = 0; i < min(local.sub.size(), remote.sub.size()); i++) {
      if (local.sub[i] != remote.sub[i]) {
        fs::remove_all(get_tmp_index_path(path, i));
      }
    }
    for (int i = remote.sub.size(); i < local.sub.size(); i++) {
      fs::remove_all(get_tmp_index_path(path, i));
    }
    std::unique_lock<std::shared_mutex> local_write_lock(mutex_local_inode_cache);
    local_inode_cache[path] = remote;
  }

  void SDK::read_lock(const std::string &dst) { pathlocks.read_lock(get_tmp_path(dst)); }

  void SDK::write_lock(const std::string &dst) {
    pathlocks.write_lock(get_tmp_path(dst));
    lock_remote(dst);
  }

  void SDK::_unlock_remote(const std::string &dst) {
    std::unique_lock<std::shared_mutex> lock(mutex_remoteLocks);
    remoteLocks.erase(dst);
  }

  void SDK::unlock(const std::string &dst) {
    // read also unlock remote(no effection)
    _unlock_remote(dst);
    pathlocks.unlock(get_tmp_path(dst));
  }

  void SDK::open(const std::string &path, int flags) {
    VLOG(2) << "open flags: " << flags << endl;

    if (flags & O_CREAT) {
      // ref mknod & create in libfuse
      _create(path);
    }

    string local_path = get_tmp_path(path);
    mkdir_f(local_path);
    if ((flags & O_ACCMODE) != O_RDONLY) {
      string write_path = get_tmp_write_path(path);
      write_lock(path);
      mkdir_f(write_path);
      clear_dir(write_path);
    } else {
      read_lock(path);
    }
    update_inode(path);

    if (flags & O_TRUNC) {
      LOG(WARNING) << "open with flag: O_TRUNC";
      _truncate(path, 0);
    }
  }

  void SDK::lock_remote(const std::string &path) {
    _lock_remote(path);
    std::unique_lock<std::shared_mutex> lock(mutex_remoteLocks);
    remoteLocks.insert(path);
  }

  void SDK::_lock_remote(const std::string &path) {
    vector<string> paths = {path};
    _lock_remote(paths);
  }

  void SDK::_lock_remote(const std::vector<std::string> &paths) {
    brpc::Controller cntl;
    NNLockRequest request;
    CommonResponse response;
    for (auto &s : paths) {
      request.add_paths(s);
    }
    nn_master_stub_ptr->update_lock(&cntl, &request, &response, NULL);
    check_response(cntl, response.common());
  }

  void SDK::close(const std::string &dst) {
    fsync(dst);
    std::unique_lock<std::shared_mutex> local_write_lock(mutex_local_inode_cache);
    local_inode_cache.erase(dst);
    local_write_lock.unlock();
    unlock(dst);
  }

  void SDK::put(const string &srcFilePath, const string &dstFilePath,
                const std::string &storage_type) {
    shared_ptr<StorageType> storage_type_ptr = StorageType::from_string(storage_type);
    VLOG(2) << "opening file " << srcFilePath;
    ifstream ifile(srcFilePath, ios::binary);
    if (!ifile.is_open()) {
      throw runtime_error("Failed to open file" + srcFilePath);
    }
    auto filesize = fs::file_size(srcFilePath);
    VLOG(2) << "filesize: " << filesize;
    brpc::Controller cntl;
    NNPutRequest nn_put_req;
    NNPutResponse nn_put_resp;
    *(nn_put_req.mutable_path()) = dstFilePath;
    *(nn_put_req.mutable_storage_type()) = storage_type;
    nn_master_stub_ptr->put(&cntl, &nn_put_req, &nn_put_resp, NULL);
    check_response(cntl, nn_put_resp.common());

    _lock_remote(dstFilePath);
    NNPutOKRequest nnPutokReq;
    *(nnPutokReq.mutable_path()) = dstFilePath;
    nnPutokReq.set_filesize(filesize);
    uint64_t blockSize = storage_type_ptr->get_block_size();
    VLOG(2) << "using block size: " << blockSize;
    string block;
    block.resize(blockSize);
    int blockIndex = 0;

    int node_index = 0;
    int i = 0;  // used to make std::set be ordered
    while (ifile.read(&block[0], blockSize) || ifile.gcount() > 0) {
      size_t bytesRead = ifile.gcount();
      // if (bytesRead < blockSize) {
      //   block.resize(blockSize, '0');  // 使用0填充到新大小
      // }
      string res = encode_one(storage_type_ptr, block);
      nnPutokReq.add_sub(res);
      blockIndex++;
    }
    ifile.close();
    _unlock_remote(dstFilePath);
    CommonResponse response;
    cntl.Reset();
    nn_master_stub_ptr->put_ok(&cntl, &nnPutokReq, &response, NULL);
    check_response(cntl, response.common());

    VLOG(2) << "put ok";
  }

  std::string SDK::get_from_datanode(const std::string &datanode, const std::string &blkid) {
    DNGetRequest dnget_req;
    DNGetResponse dnget_resp;
    *(dnget_req.mutable_blkid()) = blkid;
    brpc::Controller cntl;
    cntl.set_timeout_ms(5000);
    get_dn_stub(datanode)->get(&cntl, &dnget_req, &dnget_resp, NULL);
    check_response(cntl, dnget_resp.common());
    VLOG(2) << datanode << " | " << blkid << " success";
    string data = dnget_resp.data();
    if (cal_sha256sum(data) != blkid) {
      throw runtime_error("sha256sum check fail");
    }
    return data;
  }

  std::string SDK::put_to_datanode(const string &datanode, const std::string &block) {
    string sha256sum = cal_sha256sum(block);
    VLOG(2) << "sha256sum:" << sha256sum;
    // 假设每个分块的大小是block.size() / k
    Controller cntl;
    cntl.set_timeout_ms(5000);
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
    std::vector<pair<std::string, std::string>> nodes_hashs;
    int node_index = 0, succ = 0;
    string sha256sum;
    int fail_count = 0;  // avoid i-- becomes endless loop
    // 上传每个编码后的分块
    for (int i = 0; i < vec.size(); i++) {
      VLOG(2) << "node[" << node_index << "]"
              << ":" << nodes[node_index];
      try {
        sha256sum = put_to_datanode(nodes[node_index], vec[i]);
        nodes_hashs.push_back(make_pair(nodes[node_index], sha256sum));
        succ++;
        VLOG(2) << nodes[node_index] << " success";
      } catch (const exception &e) {
        LOG(ERROR) << e.what();
        LOG(ERROR) << nodes[node_index] << " failed";
        i--;
        fail_count++;
      }
      node_index = (node_index + 1) % nodes.size();
      if (node_index == 0 && storage_type_ptr->check(succ) == false) {
        LOG(WARNING) << "may not be secure";
        if (fail_count == nodes.size()) {
          LOG(ERROR) << "all failed";
          break;
        }
        fail_count = 0;
      }
    }
    return encode_one_sub(nodes_hashs);
  }

  std::string SDK::decode_one(std::shared_ptr<StorageType> storage_type_ptr, const std::string &s) {
    int succ = 0;
    vector<string> datas;
    datas.resize(storage_type_ptr->getDecodeBlocks());
    std::vector<std::pair<std::string, std::string>> res;
    res = decode_one_sub(s);
    string node;
    string blkid;
    for (auto &p : res) {
      node = p.first;
      blkid = p.second;
      try {
        datas[succ++] = get_from_datanode(node, blkid);
        if (succ == storage_type_ptr->getDecodeBlocks()) {
          return storage_type_ptr->decode(datas);
        }
      } catch (const exception &e) {
        LOG(ERROR) << e.what();
        LOG(ERROR) << node << " | " << blkid << " failed";
      }
    }
    return "";
  }

  void SDK::get(const std::string &src, const std::string &dst) {
    Inode inode = get_remote_inode(src);

    VLOG(2) << "using storage type: " << inode.storage_type_ptr->to_string();
    ofstream dstFile(dst, std::ios::binary);
    if (!dstFile.is_open()) {
      throw runtime_error("Failed to open file" + dst);
    }
    for (const auto &s : inode.sub) {
      dstFile << decode_one(inode.storage_type_ptr, s);
    }
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
  }

  // void SDK::local_truncate(const Inode& inode, size_t size) {
  //   inode.get_block_size();
  //   inode.filesize;
  //   for (const auto &entry : fs::directory_iterator(write_dst)) {
  //     const auto index_str = entry.path().filename().string();
  //     if (entry.is_directory()) {
  //     }
  //     res.push_back(stoi(index_str));
  //   }
  //   inode.get_block_size();
  // }
  void SDK::truncate(const std::string &dst, size_t size) {
    open(dst, O_WRONLY | O_TRUNC);
    unlock(dst);
  }
  void SDK::_truncate(const std::string &dst, size_t size) {
    Inode inode = get_inode(dst);
    if (inode.filesize == size) {
      return;
    }
    NNPutOKRequest nnPutokReq;
    *(nnPutokReq.mutable_path()) = dst;
    nnPutokReq.set_filesize(size);
    for (int i = 0; i < align_up(size, inode.get_block_size()); i++) {
      nnPutokReq.add_sub(inode.sub[i]);
    }
    Controller cntl;
    CommonResponse response;
    nn_master_stub_ptr->put_ok(&cntl, &nnPutokReq, &response, NULL);
    check_response(cntl, response.common());
    unlock(dst);
  }

  inline pair<int, int> SDK::get_indexs(const Inode &inode, uint64_t offset, size_t size) const {
    return make_pair(align_index_down(offset, inode.get_block_size()),
                     align_index_up(offset + size, inode.get_block_size()));
  }

  void SDK::ln_path_index(const std::string &path, uint32_t index) const {
    string hard_ln_path = get_tmp_write_path(path) + "/" + std::to_string(index);
    VLOG(2) << "creating hardlink: " << hard_ln_path << " >>>>> "
            << get_tmp_index_path(path, index);
    fs::remove(hard_ln_path);
    std::filesystem::create_hard_link(get_tmp_index_path(path, index), hard_ln_path);
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
    VLOG(2) << "using storage type: " << inode.storage_type_ptr->to_string();
    auto iter = inode.sub.begin() + indexs.first;
    std::ostringstream oss;
    string block;
    for (int index = indexs.first; index < indexs.second; index++) {
      if (iter >= inode.sub.end()) {
        oss << string((indexs.second - index) * inode.get_block_size(), '\0');
        break;
      }
      string blks = *iter;
      tmp_path = get_tmp_index_path(inode.get_fullpath(), index);
      if (fs::exists(tmp_path) && fs::file_size(tmp_path) > 0) {
        block = read_file(tmp_path);
      } else {
        pathlocks.write_lock(tmp_path);
        if (fs::exists(tmp_path) && fs::file_size(tmp_path) > 0) {
          block = read_file(tmp_path);
        } else {
          ofstream ofile(tmp_path, std::ios::binary);
          if (!ofile) {
            LOG(ERROR) << "Failed to open file for reading." << tmp_path;
            throw std::runtime_error("openfile error:" + tmp_path);
          }
          block = decode_one(inode.storage_type_ptr, blks);
          ofile << block;
          ofile.close();
        }
        pathlocks.unlock(tmp_path);
      }
      oss << block;
      iter++;
    }
    return oss.str();
  }

  std::string SDK::read_data(const string &path, uint64_t offset, size_t size) {
    string res;
    Inode inode = get_inode(path);
    if (inode.filesize < offset) {
      VLOG(2) << "out of range: filesize: " << inode.filesize << ", offset: " << offset
              << ", size: " << size;
      return "";
    }
    if (inode.filesize < offset + size) {
      size = inode.filesize - offset;
    }
    pair<int, int> indexs = get_indexs(inode, offset, size);
    auto s = read_data(inode, indexs);
    return string(s.begin() + offset - indexs.first * inode.get_block_size(),
                  s.begin() + offset - indexs.first * inode.get_block_size() + size);
  }

  void SDK::write_data(const string &path, uint64_t offset, const std::string &s) {
    Inode inode = get_inode(path);

    size_t size = s.size();
    auto iter = s.begin();

    pair<int, int> indexs = get_indexs(inode, offset, size);
    if (offset < inode.filesize && offset % inode.get_block_size() != 0) {
      read_data(inode, make_pair(indexs.first, indexs.first + 1));
    }
    if (offset + size < inode.filesize && indexs.first != indexs.second - 1
        && (offset + size) % inode.get_block_size() != 0) {
      read_data(inode, make_pair(indexs.second - 1, indexs.second));
    }

    for (int index = indexs.first; index < indexs.second; index++) {
      size_t left = index == indexs.first ? offset % inode.get_block_size() : 0;
      size_t right = index == indexs.second - 1 ? (offset + size) % inode.get_block_size()
                                                : inode.get_block_size();
      size_t localSize = right - left;
      // iter += localSize;

      string dst = get_tmp_index_path(path, index);

      // using ios::binary only will clear the file
      ofstream dstFile(dst, ios::binary | ios::app);
      if (!dstFile) {
        LOG(ERROR) << "failed to write data to " << dst << std::endl;
        throw runtime_error("failed to write data to " + dst);
      }
      VLOG(2) << "seek begin: " << left << ", seek end: " << right;
      dstFile.seekp(left);
      dstFile << string(iter, iter + localSize);
      dstFile.close();
      iter += localSize;
      ln_path_index(path, index);
    }
  }
  void SDK::create(const string &path) { open(path, O_WRONLY | O_CREAT); }
  // stateless
  void SDK::_create(const string &path) {
    string dst_path = get_tmp_index_path("", 0);
    std::ofstream ofile(dst_path);
    ofile.close();
    put(dst_path, path, guess_storage_type());
  }

  void SDK::fsync(const std::string &dst) {
    string write_dst = get_tmp_write_path(dst);
    if (!fs::exists(write_dst)) {
      return;
    }
    if (!fs::is_directory(write_dst)) {
      throw runtime_error(write_dst + " is not directory");
    }
    vector<int> res;
    for (auto &file : list_dir(write_dst)) {
      res.push_back(stoi(file));
    };
    if (res.empty()) return;
    sort(res.begin(), res.end());
    Inode inode = get_inode(dst);
    if (inode.sub.size() <= res.back()) {
      inode.sub.resize(res.back() + 1);
    }
    int index_filesize, last_index_filesize = 0;
    string index_block;
    for (auto &i : res) {
      string index_file_path = write_dst + "/" + std::to_string(i);
      pathlocks.read_lock(index_file_path);
      index_block = read_file(index_file_path);
      last_index_filesize = index_block.size();
      inode.sub[i] = encode_one(inode.storage_type_ptr, index_block);
      fs::remove(index_file_path);
      pathlocks.unlock(index_file_path);
    }
    int filesize = std::max(inode.filesize,
                            (uint64_t)res.back() * inode.get_block_size() + last_index_filesize);
    Controller cntl;
    NNPutOKRequest nnPutokReq;
    CommonResponse response;
    *(nnPutokReq.mutable_path()) = dst;
    nnPutokReq.set_filesize(filesize);
    for (auto &s : inode.sub) {
      nnPutokReq.add_sub(s);
    }
    nn_master_stub_ptr->put_ok(&cntl, &nnPutokReq, &response, NULL);
  }
}  // namespace spkdfs