#include "node/raft_dn.h"

#include <brpc/closure_guard.h>
#include <butil/logging.h>

#include <algorithm>
#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

#include "backward.hpp"
#include "common/node.h"
#include "dbg.h"
#include "node/config.h"
namespace spkdfs {
  using namespace std;
  using namespace dbg;
  using json = nlohmann::json;

  RaftDN::RaftDN(const std::vector<Node>& _nodes, DNApplyCallbackType applyCallback)
      : applyCallback(applyCallback) {
    node_list = _nodes;
    for (auto& node : node_list) {
      node.port = FLAGS_dn_port;
    }

    node_options.fsm = this;
    node_options.node_owns_fsm = false;
    std::string prefix = "local://" + FLAGS_data_dir + "/datanode";
    node_options.log_uri = prefix + "/log";
    node_options.raft_meta_uri = prefix + "/raft_meta";
    node_options.snapshot_uri = prefix + "/snapshot";
    node_options.disable_cli = true;
    vector<braft::PeerId> peer_ids;
    peer_ids.reserve(node_list.size());  // 优化：预先分配所需空间
    std::transform(node_list.begin(), node_list.end(), std::back_inserter(peer_ids),
                   [](const Node& node) { return node.to_peerid(); });
    node_options.initial_conf = braft::Configuration(peer_ids);

    butil::EndPoint addr(butil::my_ip(), FLAGS_dn_port);
    raft_node = new braft::Node("RaftDN", braft::PeerId(addr));
  }

  RaftDN::~RaftDN() { delete raft_node; }

  void RaftDN::start() {
    // 实现细节
    if (raft_node->init(node_options) != 0) {
      LOG(ERROR) << "Fail to init raft node" << endl;
      throw runtime_error("raft datanode init failed");
    }
  }

  void RaftDN::shutdown() { raft_node->shutdown(NULL); }

  void RaftDN::on_apply(braft::Iterator& iter) {
    // 实现细节
    butil::IOBuf data;
    for (; iter.valid(); iter.next()) {
      data = iter.data();
      braft::AsyncClosureGuard closure_guard(iter.done());
    }
    string data_str = data.to_string();
    LOG(INFO) << "got namenodes:" << data_str << endl;
    auto _json = json::parse(data_str);
    namenode_list = _json.get<vector<Node>>();
    applyCallback(namenode_list);
  }

  vector<Node> RaftDN::get_namenodes() { return namenode_list; }
  void RaftDN::on_leader_start(int64_t term) {
    backward::SignalHandling sh;
    LOG(INFO) << "Node becomes leader" << endl;
    vector<Node> nodes = get_namenodes();
    std::sort(nodes.begin(), nodes.end());

    node_discovery(nodes);

    nodes.erase(
        std::remove_if(nodes.begin(), nodes.end(),
                       [](const Node& node) { return node.nodeStatus != NodeStatus::ONLINE; }),
        nodes.end());
    LOG(INFO) << "nodes status: " << endl;
    pretty_print(LOG(INFO), nodes);
    vector<Node> allnodes = node_list;

    dbg(allnodes);
    // 从 allnodes 中删除在 nodes 中的节点
    allnodes.erase(std::remove_if(allnodes.begin(), allnodes.end(),
                                  [&nodes](const Node& node) {
                                    return std::binary_search(nodes.begin(), nodes.end(), node);
                                  }),
                   allnodes.end());

    // 遍历 allnodes
    for (size_t i = 0; i < allnodes.size() && nodes.size() < FLAGS_expected_nn;
         i += FLAGS_expected_nn) {
      size_t end = std::min(i + FLAGS_expected_nn, allnodes.size());
      std::vector<Node> sub(allnodes.begin() + i, allnodes.begin() + end);

      node_discovery(sub);

      for (auto& node : sub) {
        if (node.nodeStatus == NodeStatus::ONLINE) {
          node.port = FLAGS_nn_port;
          nodes.push_back(node);
          if (nodes.size() >= FLAGS_expected_nn) {
            break;
          }
        }
      }
    }
    dbg(nodes);
    if (nodes.empty()) {
      LOG(ERROR) << "no node ready" << endl;
      throw runtime_error("no node ready");
    }

    json j = json::array();
    for (const auto& node : nodes) {
      j.push_back(node);  // 这里会自动调用 to_json 函数
    }
    butil::IOBuf buf;
    buf.append(j.dump());
    // butil::IOBuf buf;
    braft::Task task;
    task.data = &buf;
    task.done = NULL;
    raft_node->apply(task);
  }

  // void RaftDN::on_leader_stop(const butil::Status& status) {
  //   LOG(INFO) << "Node stepped down : " << status<< endl;
  // }
  void RaftDN::on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    string file_path = writer->get_path() + "/namenodes.json";
    ofstream file(file_path, fstream::out);
    if (!file) {
      LOG(ERROR) << "Failed to open file for writing." << endl;
      done->status().set_error(EIO, "Fail to save " + file_path);
    }
    json j = json::array();
    for (const auto& node : namenode_list) {
      j.push_back(node);  // 这里会自动调用 to_json 函数
    }
    file << j.dump();
    file.close();
  };
  int RaftDN::on_snapshot_load(braft::SnapshotReader* reader) {
    string file_path = reader->get_path() + "namenodes.json";
    string str;
    ifstream file(file_path, std::fstream::in | std::fstream::out | std::fstream::app);
    if (!file) {
      LOG(ERROR) << "Failed to open file for reading." << endl;
      return -1;
    }
    file >> str;
    file.close();
    if (!str.empty()) {
      auto _json = json::parse(str);
      namenode_list = _json.get<vector<Node>>();
    }
    dbg(namenode_list);
  }

  // void RaftDN::on_shutdown() { LOG(INFO) << "This node is down" << endl; }
  // void RaftDN::on_error(const ::braft::Error& e) { LOG(ERROR) << "Met raft error " << e << endl;
  // } void RaftDN::on_configuration_committed(const ::braft::Configuration& conf) {
  //   LOG(INFO) << "Configuration of this group is " << conf;
  // }
  // void RaftDN::on_stop_following(const ::braft::LeaderChangeContext& ctx) {
  //   LOG(INFO) << "Node stops following " << ctx;
  // }
  // void RaftDN::on_start_following(const ::braft::LeaderChangeContext& ctx) {
  //   LOG(INFO) << "Node start following " << ctx;
  // }
}  // namespace spkdfs