#ifndef UTILS_H
#define UTILS_H
#include <string>
namespace spkdfs {
  inline int align_index_up(int n, int alignment) { return (n + alignment - 1) / alignment; };
  inline int align_index_down(int n, int alignment) { return n / alignment; };
  void createDirectoryIfNotExist(const std::string& dir);
  std::string cal_sha256sum(std::string);
}  // namespace spkdfs
#endif