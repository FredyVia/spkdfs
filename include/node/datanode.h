#ifndef DATANODE_H
#define DATANODE_H
#include <brpc/channel.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "common/node.h"
#include "node/sqlitedb.h"
#include "service.pb.h"
#include "node/raft_dn.h"

namespace spkdfs {
  class DatanodeServiceImpl : public DatanodeService {
  public:
    DatanodeServiceImpl(RaftDN* dn_raft_ptr) : dn_raft_ptr(dn_raft_ptr){};
    void set_namenode_master(Node node);
    void put(::google::protobuf::RpcController* controller, const DNPutRequest* request,
             CommonResponse* response, ::google::protobuf::Closure* done) override;
    void get(::google::protobuf::RpcController* controller, const DNGetRequest* request,
             DNGetResponse* response, ::google::protobuf::Closure* done) override;
    void get_namenodes(::google::protobuf::RpcController* controller, const Request* request,
                       DNGetNamenodesResponse* response,
                       ::google::protobuf::Closure* done) override;
    // void get_datanodes(::google::protobuf::RpcController* controller, const Request* request,
    //                    DNGetDatanodesResponse* response,
    //                    ::google::protobuf::Closure* done) override;

  private:
    void check_status();
    RaftDN* dn_raft_ptr;
    Node namenode_master;
    brpc::Channel channel;
    std::unique_ptr<NamenodeService_Stub> stub_ptr;
  };

}  // namespace spkdfs
#endif