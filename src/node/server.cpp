#include "node/server.h"

#include <glog/logging.h>

#include <cstddef>
#include <exception>
#include <memory>

#include "common/node.h"

namespace spkdfs {
  using namespace std;
  using namespace braft;

  Server::Server(const std::vector<Node>& nodes) {
    dn_service_ptr = new DatanodeServiceImpl([this]() {
      if (dn_raft_ptr == nullptr) {
        throw runtime_error("raft datanode not ok");
      }
      return dn_raft_ptr->get_namenodes();
    });
    dn_raft_ptr
        = new RaftDN(nodes, std::bind(&Server::on_namenodes_change, this, std::placeholders::_1));
    if (dn_server.AddService(dn_service_ptr, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
      throw runtime_error("server failed to add dn service");
    }
    if (add_service(&dn_server, FLAGS_dn_port) != 0) {
      throw runtime_error("server failed to add dn service");
    }
  }
  void Server::on_namenodes_change(const std::vector<Node>& namenodes) {
    bool found = any_of(namenodes.begin(), namenodes.end(),
                        [](const Node& node) { return node.ip == butil::my_ip_cstr(); });
    if (found && nn_raft_ptr != nullptr) {
      LOG(INFO) << "change_peers" << to_string(namenodes);
      nn_raft_ptr->change_peers(namenodes);
      return;
    }
    if (nn_raft_ptr != nullptr) {
      nn_raft_ptr->shutdown();
      nn_server.Stop(0);
      nn_server.Join();
      delete nn_raft_ptr;
      nn_raft_ptr == nullptr;
      nn_server.ClearServices();
    }
    if (found) {
      // IsLeaderCallbackType isLeaderCallback,
      //                   NNGetLeaderCallbackType getLeaderCallback,
      //                   NNApplyCallbackType applyCallback
      nn_service_ptr = new NamenodeServiceImpl(
          [this]() {
            return this->nn_raft_ptr == nullptr ? Node::INVALID_NODE : this->nn_raft_ptr->leader();
          },
          [this](const Task& task) {
            if (nn_raft_ptr) {
              nn_raft_ptr->apply(task);
            } else {
              throw runtime_error("nn raft not running");
            }
          });
      nn_raft_ptr = new RaftNN(namenodes);

      if (nn_server.AddService(nn_service_ptr, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        throw runtime_error("server failed to add nn service");
      }
      if (add_service(&nn_server, FLAGS_nn_port) != 0) {
        throw runtime_error("server failed to add nn service");
      }
      if (nn_server.Start(FLAGS_nn_port, NULL) != 0) {
        throw runtime_error("server failed to add nn service");
      }
    }
  }
  void Server::start() {
    if (dn_server.Start(FLAGS_dn_port, NULL) != 0) {
      LOG(ERROR) << "Fail to start Server" << endl;
      throw runtime_error("start service failed");
    }
    dn_raft_ptr->start();
    int count = 0;
    while (!brpc::IsAskedToQuit()) {
      sleep(1);
      count++;
      if (count == 5) {
        count = 0;
        LOG(INFO) << "Running" << endl;
      }
    }
  }
  Node Server::leader() {
    if (nn_raft_ptr) {
      return nn_raft_ptr->leader();
    }
    throw runtime_error("not running namenode");
  }
  Server::~Server() {
    nn_server.Stop(0);
    nn_server.Join();

    dn_server.Stop(0);
    dn_server.Join();

    if (nn_service_ptr) {
      delete nn_service_ptr;
      nn_service_ptr = nullptr;
    }
    if (dn_service_ptr) {
      delete nn_service_ptr;
      nn_service_ptr = nullptr;
    }
    if (nn_raft_ptr) {
      nn_raft_ptr->shutdown();
      delete nn_raft_ptr;
      nn_raft_ptr = nullptr;
    }
    if (dn_raft_ptr) {
      dn_raft_ptr->shutdown();
      delete dn_raft_ptr;
      dn_raft_ptr = nullptr;
    }
  }
}  // namespace spkdfs