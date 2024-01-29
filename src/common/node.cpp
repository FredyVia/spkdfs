#include "common/node.h"

#include <arpa/inet.h>
#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller
#include <brpc/server.h>      // brpc::Server

#include <exception>
#include <functional>
#include <thread>

#include "common/config.h"
#include "service.pb.h"

namespace spkdfs {
  using namespace std;
  using namespace braft;
  Node Node::INVALID_NODE = Node("0.0.0.0", 0);

  void Node::scan() {
    brpc::Channel channel;
    // 初始化 channel
    if (channel.Init(to_string(*this).c_str(), NULL) != 0) {
      throw runtime_error("channel init failed");
    }

    NamenodeService_Stub stub(&channel);
    brpc::Controller cntl;
    Request request;
    GetMasterResponse response;

    stub.get_master(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
      nodeStatus = NodeStatus::OFFLINE;
    } else {
      nodeStatus = NodeStatus::ONLINE;
    }
  }

  braft::PeerId Node::to_peerid() const {
    butil::EndPoint ep;
    butil::str2endpoint(ip.c_str(), port, &ep);
    braft::PeerId peerid(ep);
    return peerid;
  }
  bool Node::operator==(const Node& other) const { return ip == other.ip && port == other.port; }
  bool Node::operator!=(const Node& other) const { return !(*this == other); }
  bool Node::operator<(const Node& other) const { return ip < other.ip && port < other.port; }
  void Node::toNNNode() { port = FLAGS_nn_port; }
  void Node::toDNNode() { port = FLAGS_dn_port; }
  bool Node::valid() { return *this == INVALID_NODE; }

  void node_discovery(std::vector<Node>& nodes) {
    std::vector<std::thread> threads;

    for (auto& node : nodes) {
      threads.emplace_back(std::bind(&Node::scan, &node));
    }

    for (auto& t : threads) {
      t.join();
    }
  }

  std::string to_string(const vector<Node>& nodes) {
    std::ostringstream oss;
    for (const auto& node : nodes) {
      oss << to_string(node) << ",";
    }
    string s = oss.str();
    s.pop_back();
    return s;
  }

  vector<Node> parse_nodes(const string& nodes_str) {
    vector<Node> res;
    istringstream stream(nodes_str);
    string token;
    Node node;
    while (getline(stream, token, ',')) {
      from_string(token, node);
      res.push_back(node);
    }
    return res;
  }

  void from_peerId(braft::PeerId peerid, Node& node) {
    node.ip = inet_ntoa(peerid.addr.ip);
    node.port = peerid.addr.port;
  }
}  // namespace spkdfs