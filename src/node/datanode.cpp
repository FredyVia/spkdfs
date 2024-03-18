#include "node/datanode.h"

#include <brpc/closure_guard.h>
#include <brpc/controller.h>  // brpc::Controller
#include <sys/syslog.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "common/exception.h"
#include "common/utils.h"
#include "node/config.h"
namespace spkdfs {
  using namespace std;
  namespace fs = std::filesystem;
  // void DatanodeServiceImpl::set_namenode_master(Node node) {
  //   node.port = FLAGS_nn_port;
  //   if (node == namenode_master) return;
  //   namenode_master = node;
  //   if (channel.Init(to_string(namenode_master).c_str(), NULL) != 0) {
  //     throw runtime_error("Fail to init channel to " + to_string(namenode_master));
  //   }
  //   stub_ptr.reset(new NamenodeService_Stub(&channel));
  // }

  // void DatanodeServiceImpl::check_status() {
  //   if (namenode_master.valid()) {
  //     throw  runtime_error("NODE NEED WAIT");
  //   }
  // }

  void DatanodeServiceImpl::put(::google::protobuf::RpcController* controller,
                                const DNPutRequest* request, CommonResponse* response,
                                ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      // check_status();
      auto file_path = FLAGS_data_dir + "/blk_" + request->blkid();
      ofstream file(file_path, ios::binary);
      if (!file) {
        LOG(ERROR) << "Failed to open file for writing." << file_path;
        throw runtime_error("openfile error:" + file_path);
      }
      file << request->data();
      file.close();  // auto close when deconstruct
      response->mutable_common()->set_success(true);
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
      response->mutable_common()->set_success(false);
      response->mutable_common()->mutable_fail_info()->set_code(COMMON_EXCEPTION);
      *(response->mutable_common()->mutable_fail_info()->mutable_message()) = e.what();
    }
  }

  void DatanodeServiceImpl::get(::google::protobuf::RpcController* controller,
                                const DNGetRequest* request, DNGetResponse* response,
                                ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    try {
      // check_status();
      auto file_path = FLAGS_data_dir + "/blk_" + request->blkid();

      *(response->mutable_data()) = read_file(file_path);

      // 将读取的数据设置到响应中
      response->mutable_common()->set_success(true);
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
      response->mutable_common()->set_success(false);
      response->mutable_common()->mutable_fail_info()->set_code(COMMON_EXCEPTION);
      *(response->mutable_common()->mutable_fail_info()->mutable_message()) = e.what();
    }
  }

  void DatanodeServiceImpl::get_namenodes(::google::protobuf::RpcController* controller,
                                          const Request* request, DNGetNamenodesResponse* response,
                                          ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    LOG(INFO) << "get_namenodes_call";
    auto nodes = dn_raft_ptr->get_namenodes();
    LOG(INFO) << to_string(nodes);
    for (const auto& node : nodes) {
      std::string node_str = to_string(node);
      response->add_nodes(node_str);
    }
    response->mutable_common()->set_success(true);
  }

  void DatanodeServiceImpl::get_datanodes(::google::protobuf::RpcController* controller,
                                          const Request* request, DNGetDatanodesResponse* response,
                                          ::google::protobuf::Closure* done) {
    try {
      brpc::ClosureGuard done_guard(done);
      LOG(INFO) << "get_datanodes_call";
      auto nodes = dn_raft_ptr->get_datanodes();
      LOG(INFO) << to_string(nodes);
      for (const auto& node : nodes) {
        std::string node_str = to_string(node);
        response->add_nodes(node_str);
      }
      response->mutable_common()->set_success(true);
    } catch (const std::exception& e) {
      LOG(ERROR) << e.what();
      response->mutable_common()->set_success(false);
      response->mutable_common()->mutable_fail_info()->set_code(COMMON_EXCEPTION);
      *(response->mutable_common()->mutable_fail_info()->mutable_message()) = e.what();
    }
  }
}  // namespace spkdfs