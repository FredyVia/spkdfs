#include "node/server.h"

#include <glog/logging.h>

#include <cstddef>
#include <exception>
#include <memory>

#include "common/node.h"
#include "common/service.h"

namespace spkdfs {
  using namespace std;
  using namespace braft;

  Server::Server(const std::vector<Node>& nodes) {
    dn_raft_ptr
        = new RaftDN(nodes, std::bind(&Server::on_namenodes_change, this, std::placeholders::_1));
    CommonServiceImpl* dn_common_service_ptr = new CommonServiceImpl();
    DatanodeServiceImpl* dn_service_ptr = new DatanodeServiceImpl(dn_raft_ptr);
    if (dn_server.AddService(dn_service_ptr, brpc::SERVER_OWNS_SERVICE) != 0) {
      throw runtime_error("server failed to add dn service");
    }
    if (dn_server.AddService(dn_common_service_ptr, brpc::SERVER_OWNS_SERVICE) != 0) {
      throw runtime_error("server failed to add dn common service");
    }
    if (add_service(&dn_server, FLAGS_dn_port) != 0) {
      throw runtime_error("server failed to add dn service");
    }
  }
  void Server::on_namenodes_change(const std::vector<Node>& namenodes) {
    LOG(INFO) << "butil::my_ip_cstr(): " << butil::my_ip_cstr();
    bool found = any_of(namenodes.begin(), namenodes.end(),
                        [](const Node& node) { return node.ip == butil::my_ip_cstr(); });
    if (found) {
      LOG(INFO) << "I'm in namenodes list";
      if (nn_raft_ptr != nullptr) {
        LOG(INFO) << "change_peers:" << to_string(namenodes);
        nn_raft_ptr->change_peers(namenodes);
        return;
      }
      LOG(INFO) << "start new namenode";
      nn_raft_ptr = new RaftNN(namenodes);
      CommonServiceImpl* nn_common_service_ptr = new CommonServiceImpl();
      NamenodeServiceImpl* nn_service_ptr = new NamenodeServiceImpl(nn_raft_ptr);

      if (nn_server.AddService(nn_common_service_ptr, brpc::SERVER_OWNS_SERVICE) != 0) {
        throw runtime_error("server failed to add common nn service");
      }
      if (nn_server.AddService(nn_service_ptr, brpc::SERVER_OWNS_SERVICE) != 0) {
        throw runtime_error("server failed to add nn service");
      }
      if (add_service(&nn_server, FLAGS_nn_port) != 0) {
        throw runtime_error("server failed to add nn service");
      }
      if (nn_server.Start(FLAGS_nn_port, NULL) != 0) {
        throw runtime_error("nn server failed to start service");
      }
      nn_raft_ptr->start();
      return;
    }
    if (nn_raft_ptr != nullptr) {
      LOG(INFO) << "I'm going to stop namenode";
      nn_raft_ptr->shutdown();
      nn_server.Stop(0);
      nn_server.Join();
      delete nn_raft_ptr;
      nn_raft_ptr = nullptr;
      nn_server.ClearServices();
    }
  }
  void Server::waiting_for_rpc() {
    while (true) {
      brpc::Channel channel;
      brpc::ChannelOptions options;
      if (channel.Init(butil::my_ip_cstr(), FLAGS_dn_port, &options) != 0) {
        throw runtime_error("init channel failed");
      }
      brpc::Controller cntl;
      Request request;
      CommonResponse response;
      CommonService_Stub stub(&channel);

      stub.echo(&cntl, &request, &response, nullptr);
      sleep(3);

      if (!cntl.Failed()) break;
    }
  }
  void Server::start() {
    if (dn_server.Start(FLAGS_dn_port, NULL) != 0) {
      LOG(ERROR) << "Fail to start Server";
      throw runtime_error("start service failed");
    }
    // waiting_for_rpc();
    dn_raft_ptr->start();
    int count = 0;
    while (!brpc::IsAskedToQuit()) {
      sleep(10);
      count++;
      if (count == 5) {
        count = 0;
        LOG(INFO) << "Running";
      }
    }
  }
  Node Server::leader() {
    if (nn_raft_ptr == nullptr) {
      throw runtime_error("not running namenode");
    }
    return nn_raft_ptr->leader();
  }
  Server::~Server() {
    nn_server.Stop(0);
    nn_server.Join();

    dn_server.Stop(0);
    dn_server.Join();
    if (nn_raft_ptr != nullptr) {
      nn_raft_ptr->shutdown();
      delete nn_raft_ptr;
      nn_raft_ptr = nullptr;
    }
    if (dn_raft_ptr != nullptr) {
      dn_raft_ptr->shutdown();
      delete dn_raft_ptr;
      dn_raft_ptr = nullptr;
    }
  }
}  // namespace spkdfs