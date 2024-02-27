#ifndef UTILS_H
#define UTILS_H
#include <string>
namespace spkdfs {
  void createDirectoryIfNotExist(const std::string& dir);
  std::string cal_sha256sum(std::string);
}  // namespace spkdfs
#endif