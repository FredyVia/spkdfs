#ifndef RAFTDN_H
#define RAFTDN_H

#include <bits/types/error_t.h>
#include <braft/protobuf_file.h>  // braft::ProtoBufFile
#include <braft/raft.h>           // braft::Node braft::StateMachine
#include <braft/storage.h>        // braft::SnapshotWriter
#include <braft/util.h>           // braft::AsyncClosureGuard
#include <brpc/closure_guard.h>
#include <common/node.h>

#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "common/node.h"
#include "exception"

namespace spkdfs {

  using RunFuncType = std::function<void()>;
  using TimeoutFuncType = std::function<void(const void*)>;
  class NNTimer {
  private:
    uint interval = 3;
    bool running = true;
    std::thread* t = nullptr;
    RunFuncType runFunc;
    TimeoutFuncType timeoutFunc;

  public:
    NNTimer(uint interval, const RunFuncType& runFunc, const TimeoutFuncType& timeoutFunc);
    ~NNTimer();
    class TimeoutException : std::exception {
    private:
      const std::string& errorinfo;
      const void* data;

    public:
      TimeoutException(const std::string& errorinfo, const void* data)
          : errorinfo(errorinfo), data(data) {}
      inline virtual const char* what(void) const noexcept override { return errorinfo.c_str(); }
      inline const void* get_data() const { return data; }
    };
  };
  using DNApplyCallbackType = std::function<void(const std::vector<Node>&)>;
  class RaftDN : public braft::StateMachine {
  private:
    std::string my_ip;
    braft::Node* volatile raft_node;
    std::vector<Node> namenode_list;
    std::vector<Node> datanode_list;

    braft::NodeOptions node_options;
    // void leaderStartCallbackType(const Node& master);
    DNApplyCallbackType applyCallback;
    NNTimer* nn_timer = nullptr;

  public:
    RaftDN(const std::string& my_ip, const std::vector<Node>& nodes,
           DNApplyCallbackType applyCallback);
    ~RaftDN();

    void start();
    void shutdown();
    Node leader();
    void on_nodes_loss(const std::vector<Node>& node);

    std::vector<Node> get_namenodes();
    std::vector<Node> get_datanodes();

    void inline set_namenodes(const std::vector<Node>& nodes) { namenode_list = nodes; }
    void add_node(const Node& node);
    void remove_node(const Node& node);
    void on_apply(braft::Iterator& iter) override;
    // void on_shutdown() override;
    void on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) override;
    int on_snapshot_load(braft::SnapshotReader* reader) override;
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
