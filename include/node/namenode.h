#ifndef NAMENODE_H
#define NAMENODE_H
#include <functional>
#include <memory>
#include <string>

#include "common/node.h"
#include "node/raft_nn.h"
#include "node/rocksdb.h"
#include "service.pb.h"

namespace spkdfs {
  class NamenodeServiceImpl : public NamenodeService {
  public:
    NamenodeServiceImpl(RaftNN* nn_raft_ptr) : nn_raft_ptr(nn_raft_ptr) {}
    void ls(::google::protobuf::RpcController* controller, const NNPathRequest* request,
            NNLsResponse* response, ::google::protobuf::Closure* done) override;
    void mkdir(::google::protobuf::RpcController* controller, const NNPathRequest* request,
               CommonResponse* response, ::google::protobuf::Closure* done) override;
    void rm(::google::protobuf::RpcController* controller, const NNPathRequest* request,
            CommonResponse* response, ::google::protobuf::Closure* done) override;
    void put(::google::protobuf::RpcController* controller, const NNPutRequest* request,
             NNPutResponse* response, ::google::protobuf::Closure* done) override;
    void put_ok(::google::protobuf::RpcController* controller, const NNPutOKRequest* request,
                CommonResponse* response, ::google::protobuf::Closure* done) override;
    void get_master(::google::protobuf::RpcController* controller, const Request* request,
                    NNGetMasterResponse* response, ::google::protobuf::Closure* done) override;
    // void get_datanodes(::google::protobuf::RpcController* controller, const Request* request,
    //                 NNGetDatanodesResponse* response, ::google::protobuf::Closure* done)
    //                 override;

  private:
    // IsLeaderCallbackType isLeaderCallback;
    RaftNN* nn_raft_ptr;
  };
}  // namespace spkdfs
#endif