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
  class DatanodeServiceImpl : public spkdfs::DatanodeService {
  public:
    DatanodeServiceImpl(NamenodesCallbackType namenodesCallbackType)
        : namenodesCallbackType(namenodesCallbackType) {}
    void set_namenode_master(Node node);
    void put(::google::protobuf::RpcController* controller, const ::spkdfs::PutDNRequest* request,
             ::spkdfs::CommonResponse* response, ::google::protobuf::Closure* done) override;
    void get(::google::protobuf::RpcController* controller, const ::spkdfs::GetDNRequest* request,
             ::spkdfs::GetDNResponse* response, ::google::protobuf::Closure* done) override;
    void get_namenodes(::google::protobuf::RpcController* controller,
                       const ::spkdfs::Request* request, ::spkdfs::GetNamenodesDNResponse* response,
                       ::google::protobuf::Closure* done) override;

  private:
    void check_status();
    NamenodesCallbackType namenodesCallbackType;

    Node namenode_master;
    brpc::Channel channel;
    std::unique_ptr<spkdfs::NamenodeService_Stub> stub_ptr;
  };

}  // namespace spkdfs
#endif