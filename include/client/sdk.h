#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller

#include <string>
#include <utility>
#include <vector>

#include "common/inode.h"
#include "common/node.h"
#include "common/pathlocks.h"
#include "service.pb.h"

namespace spkdfs {
  void check_response(const brpc::Controller& cntl, const Response& response);
  class SDK {
  private:
    brpc::Channel nn_master_channel;
    brpc::Channel dn_channel;
    NamenodeService_Stub* nn_master_stub_ptr;
    DatanodeService_Stub* dn_stub_ptr;
    PathLocks pathlocks;
    std::string read_data(const Inode& inode, std::pair<int, int> indexs);
    inline std::pair<int, int> get_index(const Inode& inode, uint32_t offset, uint32_t size);

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
    std::string get_tmp_path(Inode inode);
    std::string read_data(const std::string& path, uint32_t offset, uint32_t size);
    std::string get_from_datanode(const std::string& datanode, const std::string& blkid);
    void put_part(const std::string& path, uint32_t offset, uint32_t size);

    Inode get_inode(const std::string& dst);
  };
};  // namespace spkdfs