#include "node/namenode.h"

#include <brpc/closure_guard.h>

#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "common/inode.h"
#include "node/raft_nn.h"
#include "service.pb.h"
namespace spkdfs {

  using namespace std;
  using namespace braft;
  using json = nlohmann::json;

  namespace fs = std::filesystem;

  void NamenodeServiceImpl::check_leader(Response* response) {
    Node leader = nn_raft_ptr->leader();
    if (!leader.valid()) {
      throw runtime_error("leader not ready");
    }
    if (leader.ip != my_ip) {
      throw MessageException(REDIRECT_TO_MASTER, leader.ip);
    }
  }

  void fail_response(Response* response, const std::exception& e) {
    LOG(ERROR) << e.what();
    google::FlushLogFiles(google::INFO);
    response->set_success(false);
    ErrorMessage errMsg;
    errMsg.set_code(COMMON_EXCEPTION);
    errMsg.set_message(e.what());
    *(response->mutable_fail_info()) = errMsg;
  }

  void fail_response(Response* response, const MessageException& e) {
    LOG(ERROR) << e.what();
    google::FlushLogFiles(google::INFO);
    response->set_success(false);
    *(response->mutable_fail_info()) = e.errorMessage();
  }

  void NamenodeServiceImpl::ls(::google::protobuf::RpcController* controller,
                               const NNPathRequest* request, NNLsResponse* response,
                               ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: ls";
      Inode inode;
      inode.set_fullpath(request->path().empty() ? "/" : request->path());
      nn_raft_ptr->ls(inode);
      response->mutable_common()->set_success(true);
      *(response->mutable_data()) = inode.value();
    } catch (const MessageException& e) {
      fail_response(response->mutable_common(), e);
    } catch (const std::exception& e) {
      fail_response(response->mutable_common(), e);
    }
  }

  void NamenodeServiceImpl::mkdir(::google::protobuf::RpcController* controller,
                                  const NNPathRequest* request, CommonResponse* response,
                                  ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: mkdir";
      check_leader(response->mutable_common());
      if (request->path().empty()) {
        throw runtime_error("parameter path required");
      }
      string path = request->path();
      if (path[0] != '/') {
        path = "/" + path;
      }

      Inode inode = Inode::get_default_dir(path);
      // check in rocksdb
      // if(inode.get_fullpath() == "/") throw MessageException("cannot mkdir /")
      nn_raft_ptr->prepare_mkdir(inode);
      // task.data

      // buf.append(inode.value());
      LOG(INFO) << "going to propose inode: " << inode.value();
      Task task;
      butil::IOBuf buf;
      buf.push_back(OpType::OP_MKDIR);
      buf.append(inode.value());
      task.data = &buf;  // task.data cannot be NULL
      task.done = new CommonClosure(response->mutable_common(), done_guard.release());
      nn_raft_ptr->apply(task);
    } catch (const MessageException& e) {
      fail_response(response->mutable_common(), e);
    } catch (const std::exception& e) {
      fail_response(response->mutable_common(), e);
    }
  }

  void NamenodeServiceImpl::rm(::google::protobuf::RpcController* controller,
                               const NNPathRequest* request, CommonResponse* response,
                               ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: rm";
      check_leader(response->mutable_common());

      if (request->path().empty()) {
        throw runtime_error("parameter path required");
      }

      Inode inode;
      inode.set_fullpath(request->path());

      nn_raft_ptr->prepare_rm(inode);
      // task.data

      // buf.append(inode.value());
      LOG(INFO) << "going to propose inode: " << inode.value();
      Task task;
      butil::IOBuf buf;
      buf.push_back(OpType::OP_RM);
      buf.append(inode.value());
      task.data = &buf;  // task.data cannot be NULL
      task.done = new CommonClosure(response->mutable_common(), done_guard.release());
      nn_raft_ptr->apply(task);
    } catch (const MessageException& e) {
      fail_response(response->mutable_common(), e);
    } catch (const std::exception& e) {
      fail_response(response->mutable_common(), e);
    }
  }

  void NamenodeServiceImpl::put(::google::protobuf::RpcController* controller,
                                const NNPutRequest* request, NNPutResponse* response,
                                ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: put";
      check_leader(response->mutable_common());
      if (request->path().empty()) {
        throw runtime_error("parameter path required");
      }
      Inode inode;
      inode.set_fullpath(request->path());
      nn_raft_ptr->prepare_put(inode);
      inode.storage_type_ptr = StorageType::from_string(request->storage_type());
      LOG(INFO) << "prepare put inode's prepare_put" << inode.value();
      Task task;
      butil::IOBuf buf;
      buf.push_back(OpType::OP_PUT);
      buf.append(inode.value());
      task.data = &buf;
      task.done = new CommonClosure(response->mutable_common(), done_guard.release());
      nn_raft_ptr->apply(task);
    } catch (const MessageException& e) {
      fail_response(response->mutable_common(), e);
    } catch (const std::exception& e) {
      fail_response(response->mutable_common(), e);
    }
  }

  void NamenodeServiceImpl::put_ok(::google::protobuf::RpcController* controller,
                                   const NNPutOKRequest* request, CommonResponse* response,
                                   ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);

    LOG(INFO) << "rpc: put_ok";
    try {
      check_leader(response->mutable_common());

      if (request->path().empty()) {
        throw runtime_error("parameter path required");
      }
      Inode inode;
      inode.set_fullpath(request->path());
      nn_raft_ptr->prepare_put_ok(inode);
      inode.sub = vector<string>(request->sub().begin(), request->sub().end());
      inode.filesize = request->filesize();
      LOG(INFO) << "inode's prepare_put_ok" << inode.value();
      Task task;
      butil::IOBuf buf;
      buf.push_back(OpType::OP_PUTOK);
      buf.append(inode.value());
      task.data = &buf;
      task.done = new CommonClosure(response->mutable_common(), done_guard.release());
      nn_raft_ptr->apply(task);
    } catch (const MessageException& e) {
      fail_response(response->mutable_common(), e);
    } catch (const std::exception& e) {
      fail_response(response->mutable_common(), e);
    }
  }

  void NamenodeServiceImpl::get_master(::google::protobuf::RpcController* controller,
                                       const Request* request, NNGetMasterResponse* response,
                                       ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: get_master";

      Node leader = nn_raft_ptr->leader();
      if (!leader.valid()) {
        throw runtime_error("leader not ready");
      }
      *(response->mutable_node()) = to_string(leader);
      response->mutable_common()->set_success(true);
    } catch (const MessageException& e) {
      fail_response(response->mutable_common(), e);
    } catch (const std::exception& e) {
      fail_response(response->mutable_common(), e);
    }
  }
  void NamenodeServiceImpl::update_lock(::google::protobuf::RpcController* controller,
                                        const NNLockRequest* request, CommonResponse* response,
                                        ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    LOG(INFO) << "rpc update_lock";
    try {
      check_leader(response->mutable_common());

      if (request->paths().empty()) {
        throw runtime_error("parameter paths required");
      }
      for (auto& s : request->paths()) {
        Inode inode;
        inode.set_fullpath(s);

        nn_raft_ptr->ls(inode);
        inode.update_ddl_lock();

        Task task;
        butil::IOBuf buf;
        buf.push_back(OpType::OP_PUT);
        buf.append(inode.value());
        task.data = &buf;
        task.done = new CommonClosure(response->mutable_common(), done_guard.release());
        nn_raft_ptr->apply(task);
      }
    } catch (const MessageException& e) {
      fail_response(response->mutable_common(), e);
    } catch (const std::exception& e) {
      fail_response(response->mutable_common(), e);
    }
    // request;
  }
}  // namespace spkdfs