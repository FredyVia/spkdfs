#include <brpc/server.h>  // brpc::Server

#include <set>
#include <string>
#include <vector>

#include "common/config.h"
#include "common/node.h"
#include "node/datanode.h"
#include "node/namenode.h"
#include "node/raft_dn.h"
#include "node/raft_nn.h"

namespace spkdfs {

  class Server {
  public:
    Server(const std::vector<Node>& nodes);
    ~Server();
    void start();

  private:
    brpc::Server nn_server;
    brpc::Server dn_server;
    DatanodeServiceImpl* dn_service_ptr;
    NamenodeServiceImpl* nn_service_ptr;
    RaftDN* dn_raft_ptr;
    RaftNN* nn_raft_ptr;
    void on_namenode_master_change(const Node& node);
    Node leader();
    void on_namenodes_change(const std::vector<Node>& namenodes);
  };
}  // namespace spkdfs