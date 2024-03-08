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
    NamenodeService_Stub* nn_master_stub_ptr;
    std::unordered_map<std::string, std::pair<brpc::Channel, DatanodeService_Stub*>> dn_stubs;
    PathLocks pathlocks;
    DatanodeService_Stub* get_dn_stub(const std::string& node);
    std::string read_data(const Inode& inode, std::pair<int, int> indexs);
    void write_data(const Inode& inode, int start_index, std::string s);
    inline std::pair<int, int> get_indexs(const Inode& inode, uint32_t offset, uint32_t size) const;
    std::string encode_one(std::shared_ptr<StorageType> storage_type_ptr, std::string block);
    std::string decode_one(std::shared_ptr<StorageType> storage_type_ptr, const std::string& s);

  public:
    std::vector<std::string> get_datanodes();
    SDK(const std::string& datanode);
    ~SDK();
    void mkdir(const std::string& dst);
    void rm(const std::string& dst);
    Inode ls(const std::string& dst);
    void put(const std::string& src, const std::string& dst, const std::string& storage_type);
    void get(const std::string& src, const std::string& dst);
    std::string get_tmp_path(Inode inode) const;
    std::string read_data(const std::string& path, uint32_t offset, uint32_t size);
    void write_data(const std::string& path, uint32_t offset, std::string s);
    std::string put_to_datanode(const std::string& datanode, std::string block);
    std::string get_from_datanode(const std::string& datanode, const std::string& blkid);
    void put_part(const std::string& path, uint32_t offset, uint32_t size);

    Inode get_inode(const std::string& dst);
  };
};  // namespace spkdfs