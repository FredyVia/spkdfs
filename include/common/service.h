#ifndef SERVICE_H
#define SERVICE_H
#include "service.pb.h"

namespace spkdfs {

  class CommonServiceImpl : public CommonService {
    void echo(::google::protobuf::RpcController* controller, const Request* request,
              CommonResponse* response, ::google::protobuf::Closure* done) override;
  };
}  // namespace spkdfs
#endif