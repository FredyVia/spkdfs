#ifndef NAMENODE_H
#define NAMENODE_H
#include <functional>
#include <memory>
#include <string>

#include "brpc/closure_guard.h"
#include "common/exception.h"
#include "common/node.h"
#include "node/raft_nn.h"
#include "node/rocksdb.h"
#include "service.pb.h"

namespace spkdfs {
  void fail_response(Response* response, const std::exception& e);
  void fail_response(Response* response, const MessageException& e);
  class CommonClosure : public braft::Closure {
    google::protobuf::Closure* _done;

  public:
    Response* response;
    CommonClosure(Response* response, google::protobuf::Closure* done)
        : response(response), _done(done) {}
    void Run() override {
      std::unique_ptr<CommonClosure> self_guard(this);
      brpc::ClosureGuard done_guard(_done);
    }
    inline void fail_response(const MessageException& e) { spkdfs::fail_response(response, e); }
    inline void fail_response(const std::exception& e) { spkdfs::fail_response(response, e); }
  };

  class NamenodeServiceImpl : public NamenodeService {
  public:
    NamenodeServiceImpl(const std::string& my_ip, RaftNN* nn_raft_ptr)
        : my_ip(my_ip), nn_raft_ptr(nn_raft_ptr) {}
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
    std::string my_ip;
    // IsLeaderCallbackType isLeaderCallback;
    RaftNN* nn_raft_ptr;
  };
}  // namespace spkdfs
#endif