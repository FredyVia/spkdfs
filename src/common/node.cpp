#include "common/node.h"

#include <arpa/inet.h>
#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller
#include <brpc/server.h>      // brpc::Server
#include <dbg.h>
#include <glog/logging.h>

#include <exception>
#include <functional>
#include <thread>

#include "service.pb.h"

namespace spkdfs {
  using namespace std;
  using namespace dbg;
  using namespace braft;
  Node Node::INVALID_NODE = Node("0.0.0.0", 0);

  braft::PeerId Node::to_peerid() const {
    butil::EndPoint ep;
    butil::str2endpoint(ip.c_str(), port, &ep);
    braft::PeerId peerid(ep);
    return peerid;
  }

  void Node::scan() {
    brpc::Channel channel;
    // 初始化 channel
    if (channel.Init(to_string(*this).c_str(), NULL) != 0) {
      throw runtime_error("channel init failed");
    }

    brpc::Controller cntl;
    Request request;
    CommonResponse response;

    cntl.set_timeout_ms(5000);
    CommonService_Stub stub(&channel);
    stub.echo(&cntl, &request, &response, NULL);

    LOG(INFO) << (*this);
    if (cntl.Failed()) {
      LOG(WARNING) << "RPC failed: " << cntl.ErrorText();
      LOG(WARNING) << "OFFLINE";
      nodeStatus = NodeStatus::OFFLINE;
    } else {
      nodeStatus = NodeStatus::ONLINE;
      LOG(INFO) << "ONLINE";
    }
  }
  bool Node::operator==(const Node& other) const { return ip == other.ip; }
  bool Node::operator!=(const Node& other) const { return !(*this == other); }
  bool Node::operator<(const Node& other) const { return ip < other.ip; }
  std::ostream& operator<<(std::ostream& out, const Node& node) {
    out << to_string(node);
    return out;
  }
  bool Node::valid() { return *this != INVALID_NODE; }

  void Node::from_peerId(braft::PeerId peerid) {
    ip = inet_ntoa(peerid.addr.ip);
    port = peerid.addr.port;
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

  void node_discovery(std::vector<Node>& nodes) {
    std::vector<std::thread> threads;

    for (auto& node : nodes) {
      threads.emplace_back(std::bind(&Node::scan, &node));
    }

    for (auto& t : threads) {
      t.join();
    }
    for (const auto& node : nodes) {
      LOG(INFO) << node;
      pretty_print(LOG(INFO), node.nodeStatus == NodeStatus::ONLINE ? "ONLINE" : "OFFLINE");
    }
  }
}  // namespace spkdfs