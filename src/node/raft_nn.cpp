#include "node/raft_nn.h"

#include <arpa/inet.h>
#include <brpc/closure_guard.h>
#include <butil/logging.h>

#include <nlohmann/json.hpp>

#include "node/config.h"
#include "node/namenode.h"
namespace spkdfs {

  using namespace std;
  using namespace spkdfs;
  using json = nlohmann::json;
  RaftNN::RaftNN(const std::string& my_ip, const vector<Node>& nodes)
      : my_ip(my_ip), db(FLAGS_data_dir + "/db") {
    node_options.fsm = this;
    node_options.node_owns_fsm = false;
    std::string prefix = "local://" + FLAGS_data_dir + "/namenode";
    node_options.log_uri = prefix + "/log";
    node_options.raft_meta_uri = prefix + "/raft_meta";
    node_options.snapshot_uri = prefix + "/snapshot";
    node_options.disable_cli = true;
    node_options.initial_conf.parse_from(to_string(nodes));
    raft_node = new braft::Node("RaftNN", Node(my_ip, FLAGS_nn_port).to_peerid());
    peers = nodes;
  }

  Node RaftNN::leader() {
    Node node;
    braft::PeerId leader = raft_node->leader_id();
    if (leader.is_empty()) {
      LOG(INFO) << "I'm leader";
      node.ip = my_ip;
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

  std::vector<Node> RaftNN::list_peers() { return peers; }

  void RaftNN::change_peers(const std::vector<Node>& namenodes) {
    string s = to_string(namenodes);
    LOG(INFO) << "change_peers: " << s;
    braft::Configuration conf;
    conf.parse_from(s);
    raft_node->change_peers(conf, NULL);
    peers = namenodes;
  }
  void RaftNN::reset_peers(const std::vector<Node>& namenodes) {
    string s = to_string(namenodes);
    LOG(INFO) << "reset_peers: " << s;
    braft::Configuration conf;
    conf.parse_from(s);
    raft_node->reset_peers(conf);
    peers = namenodes;
  }
  // bool RaftNN::is_leader() const { return raft_node->is_leader(); }

  void RaftNN::shutdown() { raft_node->shutdown(NULL); }

  void RaftNN::apply(const braft::Task& task) { raft_node->apply(task); }

  void RaftNN::on_apply(braft::Iterator& iter) {
    // 实现细节
    for (; iter.valid(); iter.next()) {
      braft::AsyncClosureGuard closure_guard(iter.done());
      LOG(INFO) << "on apply";

      butil::IOBuf data = iter.data();
      CommonClosure* c = dynamic_cast<CommonClosure*>(iter.done());
      try {
        enum OpType type = OP_UNKNOWN;
        // here must be uint8_t, or will cut more when using sizeof(enum OpType)
        data.cutn(&type, sizeof(uint8_t));
        string str_inode;
        data.copy_to(&str_inode);
        LOG(INFO) << "inode info: " << str_inode;
        auto _json = json::parse(str_inode);
        Inode inode = _json.get<Inode>();
        switch (type) {
          case OP_MKDIR:
            internal_mkdir(inode);
            break;
          case OP_PUT:
            internal_put(inode);
            break;
          case OP_PUTOK:
            internal_put_ok(inode);
            break;
          case OP_RM:
            internal_rm(inode);
            break;
          case OP_UNKNOWN:
          default:
            LOG(ERROR) << "Unknown type=" << static_cast<int>(type);
            break;
        }
        if (c != nullptr) c->succ_response();
      } catch (const MessageException& e) {
        if (c != nullptr) c->fail_response(e);
        LOG(ERROR) << e.what();
      } catch (const exception& e) {
        if (c != nullptr) c->fail_response(e);
        LOG(ERROR) << e.what();
      }
    }
  }

  // void on_shutdown() {}
  void RaftNN::on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) {
    braft::AsyncClosureGuard done_guard(done);
    db.snapshot();
  }

  int RaftNN::on_snapshot_load(braft::SnapshotReader* reader) {
    LOG(INFO) << "nn on_snapshot_load";
    db.load_snapshot();
    return 0;
  }
  // void on_leader_start(int64_t term) {}
  // void on_leader_stop(const butil::Status& status) {}
  // void on_error(const ::braft::Error& e) {}
  // void on_configuration_committed(const ::braft::Configuration& conf) {}
  // void on_configuration_committed(const ::braft::Configuration& conf, int64_t index) {}
  // void on_stop_following(const ::braft::LeaderChangeContext& ctx) {}
  // void on_start_following(const ::braft::LeaderChangeContext& ctx) {}

}  // namespace spkdfs