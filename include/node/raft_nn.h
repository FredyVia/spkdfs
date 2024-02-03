#ifndef RAFTNN_H
#define RAFTNN_H

#include <braft/protobuf_file.h>  // braft::ProtoBufFile
#include <braft/raft.h>           // braft::Node braft::StateMachine
#include <braft/storage.h>        // braft::SnapshotWriter
#include <braft/util.h>           // braft::AsyncClosureGuard

#include <string>

#include "common/node.h"
#include "node/rocksdb.h"

namespace spkdfs {
  // using LeaderChangeCallbackType = std::function<void(const std::vector<Node>&)>;

  class RaftNN : public braft::StateMachine {
  private:
    RocksDB db;
    braft::NodeOptions node_options;
    braft::Node* volatile raft_node;

    // friend class boost::serialization::access;
  public:
    void change_peers(const std::vector<Node>& namenodes);
    RaftNN(const std::vector<Node>& nodes);
    ~RaftNN();
    void start();
    // bool is_leader() const;
    Node leader();
    void shutdown();
    void apply(const braft::Task& task);

    inline void ls(Inode& inode) { db.ls(inode); };
    inline void prepare_mkdir(Inode& inode) { db.prepare_mkdir(inode); }

    void on_apply(braft::Iterator& iter) override;
    // void on_shutdown() override;
    void on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) override;
    int on_snapshot_load(braft::SnapshotReader* reader) override;
    // void on_leader_start(int64_t term) override;
    // void on_leader_stop(const butil::Status& status) override;
    // void on_error(const ::braft::Error& e) override;
    // void on_configuration_committed(const ::braft::Configuration& conf) override;
    // void on_configuration_committed(const ::braft::Configuration& conf, int64_t index) override;
    // void on_stop_following(const ::braft::LeaderChangeContext& ctx) override;
    // void on_start_following(const ::braft::LeaderChangeContext& ctx) override;
  };
}  // namespace spkdfs
#endif  // RAFTNN_H
