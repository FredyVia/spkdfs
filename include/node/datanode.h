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

namespace spkdfs {
  using NamenodesCallbackType = std::function<std::vector<Node>()>;
  class DatanodeServiceImpl : public DatanodeService {
  public:
    DatanodeServiceImpl(const NamenodesCallbackType& namenodesCallback)
        : namenodesCallback(namenodesCallback) {}
    void set_namenode_master(Node node);
    void put(::google::protobuf::RpcController* controller, const DNPutRequest* request,
             CommonResponse* response, ::google::protobuf::Closure* done) override;
    void get(::google::protobuf::RpcController* controller, const DNGetRequest* request,
             DNGetResponse* response, ::google::protobuf::Closure* done) override;
    void get_namenodes(::google::protobuf::RpcController* controller, const Request* request,
                       DNGetNamenodesResponse* response,
                       ::google::protobuf::Closure* done) override;

  private:
    void check_status();
    NamenodesCallbackType namenodesCallback;

    Node namenode_master;
    brpc::Channel channel;
    std::unique_ptr<NamenodeService_Stub> stub_ptr;
  };

}  // namespace spkdfs
#endif