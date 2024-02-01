#ifndef NAMENODE_H
#define NAMENODE_H
#include <braft/raft.h>

#include <functional>
#include <memory>
#include <string>

#include "common/node.h"
#include "node/sqlitedb.h"
#include "service.pb.h"

namespace spkdfs {
  // using IsLeaderCallbackType = std::function<bool()>;
  using NNGetLeaderCallbackType = std::function<Node()>;
  using NNApplyCallbackType = std::function<void(const braft::Task&)>;

  class NamenodeServiceImpl : public NamenodeService {
  public:
    NamenodeServiceImpl(SqliteDB& db,
                        // IsLeaderCallbackType isLeaderCallback,
                        NNGetLeaderCallbackType getLeaderCallback,
                        NNApplyCallbackType applyCallback)
        :  // isLeaderCallback(isLeaderCallback),
          getLeaderCallback(getLeaderCallback),
          applyCallback(applyCallback),
          db(db) {}
    void ls(::google::protobuf::RpcController* controller, const NNLsRequest* request,
            NNLsResponse* response, ::google::protobuf::Closure* done) override;
    void mkdir(::google::protobuf::RpcController* controller, const NNMkdirRequest* request,
               CommonResponse* response, ::google::protobuf::Closure* done) override;
    void put(::google::protobuf::RpcController* controller, const NNPutRequest* request,
             NNPutResponse* response, ::google::protobuf::Closure* done) override;
    void get(::google::protobuf::RpcController* controller, const NNGetRequest* request,
             NNGetResponse* response, ::google::protobuf::Closure* done) override;
    void put_ok(::google::protobuf::RpcController* controller, const NNPutOKRequest* request,
                CommonResponse* response, ::google::protobuf::Closure* done) override;
    void get_master(::google::protobuf::RpcController* controller, const Request* request,
                    NNGetMasterResponse* response, ::google::protobuf::Closure* done) override;

  private:
    // IsLeaderCallbackType isLeaderCallback;
    SqliteDB& db;
    NNGetLeaderCallbackType getLeaderCallback;
    NNApplyCallbackType applyCallback;
  };
}  // namespace spkdfs
#endif