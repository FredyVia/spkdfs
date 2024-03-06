#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller

#include <string>
#include <vector>

#include "common/inode.h"
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
    void mkdir(const std::string& dst);
    void rm(const std::string& dst);
    Inode ls(const std::string& dst);
    void put(const std::string& src, const std::string& dst, const std::string& storage_type);

    template <typename Iter>
    std::string decode_one(Iter begin, Iter end, std::shared_ptr<StorageType> storage_type_ptr);
    void get(const std::string& src, const std::string& dst);
    std::string get_from_datanode(const std::string& datanode, const std::string& blkid);
    std::string get_part(const std::string& path, uint32_t offset, uint32_t size);
    void put_part(const std::string& path, uint32_t offset, uint32_t size);
  };
};  // namespace spkdfs