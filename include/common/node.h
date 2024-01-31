#ifndef NODE_H
#define NODE_H
#include <braft/raft.h>

#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace spkdfs {
  enum class NodeStatus { ONLINE, OFFLINE, UNKNOWN };

  class Node {
  public:
    std::string ip;
    int port;
    NodeStatus nodeStatus;
    Node(std::string ip = "", int port = 0, NodeStatus nodeStatus = NodeStatus::UNKNOWN)
        : ip(ip), port(port), nodeStatus(nodeStatus){};
    void scan();
    // 重载 == 运算符
    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;
    bool operator<(const Node& other) const;
    bool valid();
    static Node INVALID_NODE;
    braft::PeerId to_peerid() const;
  };
  // 仅适用于具有迭代器的类型
  std::string to_string(const std::vector<Node>& nodes);

  void node_discovery(std::vector<Node>& nodes);
  std::vector<Node> parse_nodes(const std::string& nodes_str);
  inline void from_string(const std::string& node_str, Node& node) {
    std::istringstream iss(node_str);
    std::getline(iss, node.ip, ':');
    std::string portStr;
    std::getline(iss, portStr);
    node.port = portStr.empty() ? 0 : std::stoi(portStr);
  };

  inline std::string to_string(const Node& node) {
    return node.ip + ":" + std::to_string(node.port);
  };

  inline void to_json(nlohmann::json& j, const Node& node) {
    j = nlohmann::json{{"ip", node.ip}, {"port", node.port}};
  }

  inline void from_json(const nlohmann::json& j, Node& node) {
    node.ip = j.at("ip").get<std::string>();
    node.port = j.at("port").get<int>();
  }
  std::ostream& operator<<(std::ostream& out, const Node& node);
}  // namespace spkdfs

#endif