#include <sys/sysinfo.h>

#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "client/sdk.h"
using namespace std;
using namespace spkdfs;

class ThreadSafeStringSelector {
public:
  ThreadSafeStringSelector(const string& characters)
      : eng(rd()), dist(0, characters.size() - 1), chars(characters) {
    if (characters.size() < 32) {
      throw runtime_error("Characters string must be at least 16 characters long.");
    }
  }

  // 从chars中随机选择长度为16的字符串
  string select() {
    lock_guard<mutex> guard(mtx);
    string result;
    for (int i = 0; i < 32; ++i) {
      result += chars[dist(eng)];
    }
    return result;
  }

private:
  random_device rd;
  mt19937 eng;
  uniform_int_distribution<> dist;
  string chars;
  mutex mtx;
};

int main(int argc, char* argv[]) {
  cout << "This system has " << get_nprocs_conf() << " processors configured and " << get_nprocs()
       << " processors available.\n"
       << endl;
  SDK sdk("192.168.88.112:8001");
  vector<thread> threads;
  int times = 100;
  string availableChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  ThreadSafeStringSelector selector(availableChars);
  for (int i = 0; i < get_nprocs(); i++) {
    threads.emplace_back([i, &sdk, &times, &selector]() {
      for (int i = 0; i < times; i++) {
        try {
          sdk.mkdir("/" + selector.select());
        } catch (const exception& e) {
        }
      }
    });
  }
  for (int i = 0; i < get_nprocs(); i++) {
    if (threads[i].joinable()) threads[i].join();
  }
  return 0;
}