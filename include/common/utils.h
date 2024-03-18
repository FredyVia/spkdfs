#ifndef UTILS_H
#define UTILS_H
#include <string>
#include <vector>

#include "common/node.h"
namespace spkdfs {
  std::string get_my_ip(const std::vector<Node>& vec);
  inline int align_index_up(int n, int alignment) { return n / alignment + 1; };
  inline int align_index_down(int n, int alignment) { return n / alignment; };
  inline int align_up(int n, int alignment) { return (n + alignment - 1) / alignment; };
  inline int align_down(int n, int alignment) { return align_index_down(n, alignment); };
  void createDirectoryIfNotExist(const std::string& dir);
  std::string cal_sha256sum(const std::string&);
  std::string cal_md5sum(const std::string&);
  std::string simplify_path(const std::string&);
  std::string read_file(const std::string& path);
}  // namespace spkdfs
#endif