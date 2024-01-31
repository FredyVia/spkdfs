#include "common/service.h"

#include <brpc/closure_guard.h>
namespace spkdfs {
  void CommonServiceImpl::echo(::google::protobuf::RpcController* controller,
                               const Request* request, CommonResponse* response,
                               ::google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    response->mutable_common()->set_success(true);
  }
}  // namespace spkdfs