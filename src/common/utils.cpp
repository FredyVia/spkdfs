#include "common/utils.h"

#include <arpa/inet.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/sha.h>
#define DBG_MACRO_NO_WARNING
#include <dbg.h>
#include <glog/logging.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#include <filesystem>
#include <memory>
#include <set>

namespace spkdfs {
  using namespace std;
  std::set<std::string> get_all_ips() {
    std::set<std::string> res;
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *addr = nullptr;
    void *tmpAddrPtr = nullptr;

    if (getifaddrs(&interfaces) == -1) {
      throw runtime_error("get_all_ips getifaddrs failed");
    }

    LOG(INFO) << "get_all_ips:";
    for (addr = interfaces; addr != nullptr; addr = addr->ifa_next) {
      if (addr->ifa_addr == nullptr) {
        continue;
      }
      if (addr->ifa_addr->sa_family == AF_INET) {  // check it is IP4
        // is a valid IP4 Address
        tmpAddrPtr = &((struct sockaddr_in *)addr->ifa_addr)->sin_addr;
        char addressBuffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        LOG(INFO) << addr->ifa_name << " IP Address: " << addressBuffer;
        res.insert(string(addressBuffer));
        // } else if (addr->ifa_addr->sa_family == AF_INET6) {  // not support IP6 curr
        //   // is a valid IP6 Address
        //   tmpAddrPtr = &((struct sockaddr_in6 *)addr->ifa_addr)->sin6_addr;
        //   char addressBuffer[INET6_ADDRSTRLEN];
        //   inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
        //   std::cout << addr->ifa_name << " IP Address: " << addressBuffer;
      }
    }

    if (interfaces) {
      freeifaddrs(interfaces);
    }
    return res;
  }

  std::string get_my_ip(const std::vector<Node> &vec) {
    set<string> nodes;
    for (const auto &node : vec) {
      nodes.insert(node.ip);
    }
    set<string> local = get_all_ips();
    std::set<string> intersection;
    // 使用 std::set_intersection 查找交集
    std::set_intersection(nodes.begin(), nodes.end(), local.begin(), local.end(),
                          std::inserter(intersection, intersection.begin()));
    LOG(INFO) << "using ip";
    for (auto &str : intersection) {
      LOG(INFO) << str << ", ";
    }
    if (intersection.size() == 0) {
      LOG(ERROR) << "cluster ips: ";
      dbg::pretty_print(LOG(ERROR), nodes);
      LOG(ERROR) << "local ips: ";
      dbg::pretty_print(LOG(ERROR), local);
      throw runtime_error("cannot find equal ips");
    }
    return *(intersection.begin());
  }

  void createDirectoryIfNotExist(const std::string &dir) {
    filesystem::path dirPath{dir};
    error_code ec;  // 用于接收错误码

    // 创建目录，包括所有必要的父目录
    if (!filesystem::create_directories(dirPath, ec)) {
      if (ec) {
        LOG(ERROR) << "Error: Unable to create directory " << dir << ". " << ec.message();
        throw runtime_error("mkdir error: " + dir + ec.message());
      }
    }
  }
  std::string cal_sha256sum(std::string data) {
    string sha256sum;
    CryptoPP::SHA256 sha256;
    CryptoPP::StringSource ss(
        data, true,
        new CryptoPP::HashFilter(sha256,
                                 new CryptoPP::HexEncoder(new CryptoPP::StringSink(sha256sum))));
    return sha256sum;
  }

  std::string cal_md5sum(std::string data) {
    string md5sum;
    CryptoPP::Weak1::MD5 md5;
    CryptoPP::StringSource ss(
        data, true,
        new CryptoPP::HashFilter(md5, new CryptoPP::HexEncoder(new CryptoPP::StringSink(md5sum))));

    return md5sum;
  }
}  // namespace spkdfs