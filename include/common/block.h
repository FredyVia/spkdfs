#ifndef BLOCK_H
#define BLOCK_H
#include <string>

#include "common/node.h"
namespace spkdfs {

  class Block {
  public:
    std::string blk_id;
    std::string node;
  };

}  // namespace spkdfs
#endif