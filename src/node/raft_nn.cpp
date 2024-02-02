#include "node/raft_nn.h"

#include <arpa/inet.h>
#include <butil/logging.h>

#include <nlohmann/json.hpp>

#include "node/config.h"
namespace spkdfs {

  using namespace std;
  using namespace spkdfs;
  using json = nlohmann::json;
  RaftNN::RaftNN(const vector<Node>& nodes, SqliteDB& db) : db(db) {
    butil::EndPoint addr(butil::my_ip(), FLAGS_nn_port);
    node_options.fsm = this;
    node_options.node_owns_fsm = false;
    std::string prefix = "local://" + FLAGS_data_dir + "/namenode";
    node_options.log_uri = prefix + "/log";
    node_options.raft_meta_uri = prefix + "/raft_meta";
    node_options.snapshot_uri = prefix + "/snapshot";
    node_options.disable_cli = true;
    node_options.initial_conf.parse_from(to_string(nodes));
    raft_node = new braft::Node("RaftNN", braft::PeerId(addr));
  }

  Node RaftNN::leader() {
    Node node;
    braft::PeerId leader = raft_node->leader_id();
    if (leader.is_empty()) {
      LOG(INFO) << "I'm leader";
      node.ip = butil::my_ip_cstr();
      node.port = FLAGS_nn_port;
    } else {
      node.from_peerId(leader);
    }
    LOG(INFO) << "leader: " << node;
    return node;
  }

  RaftNN::~RaftNN() { delete raft_node; }

  void RaftNN::start() {
    if (raft_node->init(node_options) != 0) {
      throw runtime_error("raft namenode init failed");
    }
  }

  void RaftNN::change_peers(const std::vector<Node>& namenodes) {
    string s = to_string(namenodes);
    LOG(INFO) << s;
    braft::Configuration conf;
    conf.parse_from(s);
    raft_node->change_peers(conf, NULL);
  }

  // bool RaftNN::is_leader() const { return raft_node->is_leader(); }

  void RaftNN::shutdown() { raft_node->shutdown(NULL); }

  void RaftNN::apply(const braft::Task& task) { raft_node->apply(task); }

  void RaftNN::on_apply(braft::Iterator& iter) {
    // 实现细节
    Inode inode;
    for (; iter.valid(); iter.next()) {
      braft::AsyncClosureGuard closure_guard(iter.done());
      LOG(INFO) << "internal mkdir" << iter.data().to_string();
      auto j = json::parse(iter.data().to_string());
      inode = j.get<Inode>();
      db.internal_mkdir(inode);
    }
  }

  // void on_shutdown() {}
  // void on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) {}
  // int on_snapshot_load(braft::SnapshotReader* reader) {}
  // void on_leader_start(int64_t term) {}
  // void on_leader_stop(const butil::Status& status) {}
  // void on_error(const ::braft::Error& e) {}
  // void on_configuration_committed(const ::braft::Configuration& conf) {}
  // void on_configuration_committed(const ::braft::Configuration& conf, int64_t index) {}
  // void on_stop_following(const ::braft::LeaderChangeContext& ctx) {}
  // void on_start_following(const ::braft::LeaderChangeContext& ctx) {}

}  // namespace spkdfs