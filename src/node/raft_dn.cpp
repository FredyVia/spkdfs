#include "node/raft_dn.h"

#include <butil/logging.h>

#include <algorithm>
#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

#include "common/config.h"
#include "common/node.h"

namespace spkdfs {
  using namespace std;
  using json = nlohmann::json;

  RaftDN::RaftDN(const std::vector<Node>& nodes, DNApplyCallbackType applyCallback)
      : applyCallback(applyCallback) {
    node_list = nodes;
    for (auto& node : node_list) {
      node.port = FLAGS_dn_port;
    }

    node_options.fsm = this;
    node_options.node_owns_fsm = false;
    std::string prefix = "local://" + FLAGS_data_path + "/datanode";
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
      LOG(ERROR) << "Fail to init raft node";
      throw runtime_error("raft datanode init failed");
    }
  }

  void RaftDN::shutdown() { raft_node->shutdown(NULL); }

  void RaftDN::on_apply(braft::Iterator& iter) {
    // 实现细节
    butil::IOBuf data;
    for (; iter.valid(); iter.next()) {
      data = iter.data();
    }
    string data_str = data.to_string();
    save_namenodes(data_str);
    auto _json = json::parse(data_str);
    vector<Node> nodes = _json.get<vector<Node>>();
    applyCallback(nodes);
  }

  vector<Node> RaftDN::get_namenodes() {
    string file_path = FLAGS_data_path + "/namenodes.json";
    string str;
    ifstream file(file_path, std::fstream::in | std::fstream::out
                                 | std::fstream::app);  // 打开文件并移动到文件末尾以确定文件大小
    if (!file) {
      cerr << "Failed to open file for reading." << endl;
      throw runtime_error("openfile error:" + file_path);
    }
    file >> str;
    file.close();
    vector<Node> namenodes;
    if (!str.empty()) {
      auto _json = json::parse(str);
      namenodes = _json.get<vector<Node>>();
    }
    return namenodes;
  }

  void RaftDN::save_namenodes(const string& str) {
    std::lock_guard<std::mutex> lock(file_mutex);
    string file_path = FLAGS_data_path + "/namenodes.json";
    ofstream file(file_path, ios::binary);
    if (!file) {
      cerr << "Failed to open file for writing." << endl;
      throw runtime_error("openfile error:" + file_path);
    }
    file << str;
    file.close();
  }

  void RaftDN::on_leader_start(int64_t term) {
    LOG(INFO) << "Node becomes leader";
    vector<Node> nodes = get_namenodes();
    std::sort(nodes.begin(), nodes.end());
    vector<Node> allnodes = node_list;

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

      for (const auto& node : sub) {
        if (node.nodeStatus == NodeStatus::ONLINE) {
          nodes.push_back(node);
          if (nodes.size() >= FLAGS_expected_nn) {
            break;
          }
        }
      }
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
    return raft_node->apply(task);
  }

  // void RaftDN::on_leader_stop(const butil::Status& status) {
  //   LOG(INFO) << "Node stepped down : " << status;
  // }

  // void RaftDN::on_shutdown() { LOG(INFO) << "This node is down"; }
  // void RaftDN::on_error(const ::braft::Error& e) { LOG(ERROR) << "Met raft error " << e; }
  // void RaftDN::on_configuration_committed(const ::braft::Configuration& conf) {
  //   LOG(INFO) << "Configuration of this group is " << conf;
  // }
  // void RaftDN::on_stop_following(const ::braft::LeaderChangeContext& ctx) {
  //   LOG(INFO) << "Node stops following " << ctx;
  // }
  // void RaftDN::on_start_following(const ::braft::LeaderChangeContext& ctx) {
  //   LOG(INFO) << "Node start following " << ctx;
  // }
}  // namespace spkdfs