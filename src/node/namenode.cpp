#include "node/namenode.h"

#include <brpc/closure_guard.h>

#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "common/inode.h"
#include "exception"
#include "node/raft_nn.h"
namespace spkdfs {

  using namespace std;
  using namespace braft;
  using json = nlohmann::json;

  namespace fs = std::filesystem;
  void NamenodeServiceImpl::ls(::google::protobuf::RpcController* controller,
                               const NNLsRequest* request, NNLsResponse* response,
                               ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: ls";
      fs::path filepath(request->path().empty() ? "/" : request->path());
      Inode inode;

      inode.filename = filepath.filename().string();
      inode.parent_path = filepath.parent_path().string();

      auto inodes = db.ls(inode);
      response->mutable_common()->set_success(true);
      for (const auto& inode : inodes) {
        response->add_data(inode.filename);
      }
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
      response->mutable_common()->set_success(false);
      *(response->mutable_common()->mutable_fail_info()) = e.what();
    }
  };

  void NamenodeServiceImpl::mkdir(::google::protobuf::RpcController* controller,
                                  const NNMkdirRequest* request, CommonResponse* response,
                                  ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: mkdir";
      Node leader = getLeaderCallback();
      if (!leader.valid()) {
        throw runtime_error("leader not ready");
      }
      if (leader.ip != butil::my_ip_cstr()) {
        response->mutable_common()->set_success(false);
        *(response->mutable_common()->mutable_redirect()) = to_string(leader);
        return;
      }
      if (request->path().empty()) {
        throw runtime_error("parameter path required");
      }

      fs::path filepath(request->path());

      Inode inode;

      inode.filename = filepath.filename().string();
      inode.parent_path = filepath.parent_path().string();

      db.prepare_mkdir(inode);
      // task.data
      json j;
      to_json(j, inode);
      butil::IOBuf buf;
      buf.append(j.dump());
      Task task;
      task.data = &buf;
      task.done = NULL;
      applyCallback(task);
      response->mutable_common()->set_success(true);
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
      response->mutable_common()->set_success(false);
      *(response->mutable_common()->mutable_fail_info()) = e.what();
    }
  };
  void NamenodeServiceImpl::get(::google::protobuf::RpcController* controller,
                                const NNGetRequest* request, NNGetResponse* response,
                                ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: get";
      if (request->path().empty()) {
        throw runtime_error("parameter path required");
      }
      request->path();
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
      response->mutable_common()->set_success(false);
      *(response->mutable_common()->mutable_fail_info()) = e.what();
    }
  };
  void NamenodeServiceImpl::put(::google::protobuf::RpcController* controller,
                                const NNPutRequest* request, NNPutResponse* response,
                                ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: put";

      Node leader = getLeaderCallback();
      if (!leader.valid()) {
        throw runtime_error("leader not ready");
      }
      if (leader.ip != butil::my_ip_cstr()) {
        response->mutable_common()->set_success(false);
        *(response->mutable_common()->mutable_redirect()) = to_string(leader);
        return;
      }
      response->mutable_common()->set_success(false);
      if (request->path().empty()) {
        throw runtime_error("parameter path required");
      }
      // for (const auto& node : nodes) {
      //   std::string nodeStr = to_string(node);
      //   response->add_nodes(to_string(node));
      // }
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
      response->mutable_common()->set_success(false);
      *(response->mutable_common()->mutable_fail_info()) = e.what();
    }
  };
  void NamenodeServiceImpl::put_ok(::google::protobuf::RpcController* controller,
                                   const NNPutOKRequest* request, CommonResponse* response,
                                   ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    LOG(INFO) << "rpc: put_ok";
  }
  void NamenodeServiceImpl::get_master(::google::protobuf::RpcController* controller,
                                       const Request* request, NNGetMasterResponse* response,
                                       ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      LOG(INFO) << "rpc: get_master";

      Node leader = getLeaderCallback();
      if (!leader.valid()) {
        throw runtime_error("leader not ready");
      }
      *(response->mutable_node()) = to_string(leader);
      response->mutable_common()->set_success(true);
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
      response->mutable_common()->set_success(false);
      *(response->mutable_common()->mutable_fail_info()) = e.what();
    }
  }
}  // namespace spkdfs