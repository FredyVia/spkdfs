#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller

#include <map>
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
    brpc::Channel nn_slave_channel;
    NamenodeService_Stub* nn_master_stub_ptr;
    NamenodeService_Stub* nn_slave_stub_ptr;

    std::shared_mutex mutex_dn_stubs;
    std::unordered_map<std::string, std::pair<brpc::Channel, DatanodeService_Stub*>> dn_stubs;

    PathLocks pathlocks;

    std::shared_mutex mutex_remoteLocks;  // 用于保护锁映射的互斥量
    std::set<std::string> remoteLocks;

    std::shared_ptr<IntervalTimer> timer;

    std::map<std::string, Inode> local_inode_cache;
    std::shared_mutex mutex_local_inode_cache;

    DatanodeService_Stub* get_dn_stub(const std::string& node);
    std::string get_slave_namenode(const std::vector<std::string>& namenodes);
    std::string read_data(const Inode& inode, std::pair<int, int> indexs);
    // void write_data(const Inode& inode, int start_index, std::string s);
    inline std::pair<int, int> get_indexs(const Inode& inode, uint64_t offset, size_t size) const;
    void ln_path_index(const std::string& path, uint32_t index) const;
    // inline std::string get_ln_path_index(const std::string& path, uint32_t index) const;
    inline std::string get_tmp_write_path(const std::string& path) const;
    inline std::string get_tmp_path(const std::string& path) const;
    std::string get_tmp_index_path(const std::string& path, uint32_t index) const;
    std::string encode_one(std::shared_ptr<StorageType> storage_type_ptr, const std::string& block);
    std::string decode_one(std::shared_ptr<StorageType> storage_type_ptr, const std::string& s);
    void _lock_remote(const std::vector<std::string>& paths);
    void _lock_remote(const std::string& path);
    void _unlock_remote(const std::string& dst);
    void lock_remote(const std::string&);
    void update_inode(const std::string& path);
    Inode get_inode(const std::string& dst);
    Inode get_remote_inode(const std::string& dst);
    inline std::string guess_storage_type() const;
    std::vector<std::string> get_online_datanodes();
    std::vector<std::string> get_datanodes();
    std::string get_tmp_path(const std::string& path, uint32_t index) const;
    std::string put_to_datanode(const std::string& datanode, const std::string& block);
    std::string get_from_datanode(const std::string& datanode, const std::string& blkid);
    void _create(const std::string& path);
    void _truncate(const std::string& dst, size_t size);
    // void local_truncate(const std::string& dst, size_t size);
    void read_lock(const std::string& dst);
    void write_lock(const std::string& dst);
    void unlock(const std::string& dst);

  public:
    SDK(const std::string& datanode, const std::string& namenode = "");
    ~SDK();
    void open(const std::string& path, int flags);
    void create(const std::string& path);
    void close(const std::string& path);
    void mkdir(const std::string& dst);
    void rm(const std::string& dst);
    void truncate(const std::string& dst, size_t size);
    Inode ls(const std::string& dst);
    void put(const std::string& src, const std::string& dst, const std::string& storage_type);
    void get(const std::string& src, const std::string& dst);
    std::string read_data(const std::string& path, uint64_t offset, size_t size);
    void write_data(const std::string& path, uint64_t offset, const std::string& s);
    void fsync(const std::string& dst);
  };
};  // namespace spkdfs