#include "common/pathlocks.h"

#include <glog/logging.h>

namespace spkdfs {
  void PathLocks::lock(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mapMutex);
    LOG(INFO) << "lock: " << key;
    // 如果键不存在，则创建一个新的锁
    if (locks.find(key) == locks.end()) {
      locks[key] = std::make_unique<std::mutex>();
    }

    // 获取对应的互斥锁，但不在此处锁定
    auto& mutex = locks[key];

    // 释放映射互斥量，避免在持有它的情况下锁定资源锁，从而减少锁的粒度
    lock.unlock();

    // 锁定对应的资源
    mutex->lock();
  }

  void PathLocks::unlock(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mapMutex);  // 只读访问映射
    LOG(INFO) << "unlock: " << key;
    auto it = locks.find(key);
    if (it != locks.end()) {
      it->second->unlock();
    }
  }
}  // namespace spkdfs