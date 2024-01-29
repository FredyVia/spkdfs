#ifndef CONFIG_H
#define CONFIG_H
#include <gflags/gflags.h>

#include <vector>

#include "common/node.h"
namespace spkdfs {
  DECLARE_string(data_path);
  DECLARE_string(nodes);
  DECLARE_uint32(nn_port);
  DECLARE_uint32(dn_port);
  DECLARE_uint32(expected_nn);
  // static std::shared_ptr<Config> instance_ = nullptr;
}  // namespace spkdfs
#endif