#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller

#include <string>
#include <vector>

#include "common/node.h"
#include "service.pb.h"
namespace spkdfs {
  template <typename ResponseType>
  void check_response(const brpc::Controller& cntl, const ResponseType& response);
  class SDK {
    brpc::Channel nn_master_channel;
    brpc::Channel dn_channel;
    NamenodeService_Stub* nn_master_stub_ptr;
    DatanodeService_Stub* dn_stub_ptr;

  public:
    std::vector<Node> get_datanodes();
    SDK(const std::string& datanode);
    ~SDK();
    void mkdir(const std::string& path);
    void rm(const std::string& path);
    void ls(const std::string& path);
    void put(const std::string& src, const std::string& dst, const std::string& storage_type,
             unsigned int blocksize);
    void get(const std::string& src, const std::string& dst);
  };
};  // namespace spkdfs