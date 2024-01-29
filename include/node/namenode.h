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
    NamenodeServiceImpl(
        // IsLeaderCallbackType isLeaderCallback,
        NNGetLeaderCallbackType getLeaderCallback, NNApplyCallbackType applyCallback)
        :  // isLeaderCallback(isLeaderCallback),
          getLeaderCallback(getLeaderCallback),
          applyCallback(applyCallback) {}
    void ls(::google::protobuf::RpcController* controller, const LsNNRequest* request,
            LsNNResponse* response, ::google::protobuf::Closure* done) override;
    void mkdir(::google::protobuf::RpcController* controller, const MkdirNNRequest* request,
               CommonResponse* response, ::google::protobuf::Closure* done) override;
    void put(::google::protobuf::RpcController* controller, const PutNNRequest* request,
             PutNNResponse* response, ::google::protobuf::Closure* done) override;
    void get(::google::protobuf::RpcController* controller, const GetNNRequest* request,
             GetNNResponse* response, ::google::protobuf::Closure* done) override;
    void put_ok(::google::protobuf::RpcController* controller, const PutOKNNRequest* request,
                CommonResponse* response, ::google::protobuf::Closure* done) override;
    void get_master(::google::protobuf::RpcController* controller, const Request* request,
                    GetMasterResponse* response, ::google::protobuf::Closure* done) override;

  private:
    // IsLeaderCallbackType isLeaderCallback;
    SqliteDB db;
    NNGetLeaderCallbackType getLeaderCallback;
    NNApplyCallbackType applyCallback;
  };
  class LambdaClosure : public braft::Closure {
  public:
    using FuncType = std::function<void()>;

    explicit LambdaClosure(FuncType func) : _func(std::move(func)) {}

    void Run() override {
      if (_func) {
        _func();  // 调用 lambda 表达式
      }
      delete this;
    }

  private:
    FuncType _func;  // 存储 lambda 表达式
  };
}  // namespace spkdfs