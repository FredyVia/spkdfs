#ifndef RAFTDN_H
#define RAFTDN_H

#include <braft/protobuf_file.h>  // braft::ProtoBufFile
#include <braft/raft.h>           // braft::Node braft::StateMachine
#include <braft/storage.h>        // braft::SnapshotWriter
#include <braft/util.h>           // braft::AsyncClosureGuard
#include <common/node.h>

#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "common/node.h"

namespace spkdfs {
  using DNApplyCallbackType = std::function<void(const std::vector<Node>&)>;
  class RaftDN : public braft::StateMachine {
  private:
    braft::Node* volatile raft_node;

    std::vector<Node> node_list;

    braft::NodeOptions node_options;
    // void leaderStartCallbackType(const Node& master);
    DNApplyCallbackType applyCallback;
    std::mutex file_mutex;

  public:
    RaftDN(const std::vector<Node>& nodes, DNApplyCallbackType applyCallback);
    ~RaftDN();

    void start();
    void shutdown();
    std::vector<Node> get_namenodes();
    void save_namenodes(const std::string& str);

    void on_apply(braft::Iterator& iter) override;
    // void on_shutdown() override;
    // void on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) override;
    // int on_snapshot_load(braft::SnapshotReader* reader) override;
    void on_leader_start(int64_t term) override;
    // void on_leader_stop(const butil::Status& status) override;
    // void on_error(const ::braft::Error& e) override;
    // void on_configuration_committed(const ::braft::Configuration& conf) override;
    // void on_configuration_committed(const ::braft::Configuration& conf, int64_t index) override;
    // void on_stop_following(const ::braft::LeaderChangeContext& ctx) override;
    // void on_start_following(const ::braft::LeaderChangeContext& ctx) override;
  };
}  // namespace spkdfs
#endif  // RaftDN_H
