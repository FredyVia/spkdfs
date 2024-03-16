#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller
#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "client/sdk.h"
#include "common/inode.h"
#include "common/utils.h"
#include "service.pb.h"
using namespace std;
using namespace spkdfs;

TEST(ClientTest, encode_decode) {}

TEST(ClientTest, mkdir) {
  SDK sdk("192.168.88.112:8001");
  // should be sequencial
  vector<string> datas = {
      "/abc",
      "/abcv/",
      "/abc/abc",
  };
  map<string, set<string>> res;
  for (auto iter = datas.rbegin(); iter != datas.rend(); iter++) {
    try {
      sdk.rm(*iter);
    } catch (const exception& e) {
      cout << "rm " << *iter << e.what() << endl;
    }
  }
  for (auto& s : datas) {
    sdk.mkdir(s);
    Inode inode;
    inode.set_fullpath(s);
    res[inode.parent_path()].insert(inode.filename());
  }
  int c = 0;
  for (auto& p : res) {
    cout << p.first << endl;
    for (auto& s : p.second) {
      cout << s << ", ";
    }
    cout << endl;
    Inode inode = sdk.ls(p.first);
    set<string> sub(inode.sub.begin(), inode.sub.end());
    for (auto& s : p.second) {
      Inode inode;
      inode.set_fullpath(s);
      EXPECT_NE(sub.find(inode.filename() + "/"), sub.end()) << s << "not found";
    }
  }
}

TEST(ClientTest, concurrency) {
  SDK sdk("192.168.88.112:8001");
  vector<std::thread> threads;
  string data = "/tests/";
  try {
    sdk.rm(data);
  } catch (const exception& e) {
  }
  // 创建并启动多个线程
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back([i, &sdk, &data]() {
      try {
        sdk.mkdir(data);
      } catch (const exception& e) {
      }
    });
  }

  // 等待所有线程完成
  for (auto& t : threads) {
    t.join();
  }

  Inode inode = sdk.ls("/");
  int c = 0;
  for (auto& s : inode.sub) {
    if (s == data) {
      c++;
    }
  }
  // EXPECT_EQ(c, 1);
}
TEST(TestCommon, alignup) {
  vector<vector<int>> checks = {{2, 5, 0, 1}, {4, 5, 0, 1}, {5, 5, 1, 2}, {6, 5, 1, 2},
                                {1, 2, 0, 1}, {2, 2, 1, 2}, {4, 2, 2, 3}, {5, 2, 2, 3}};
  for (auto& vec : checks) {
    EXPECT_EQ(align_index_down(vec[0], vec[1]), vec[2]);
    EXPECT_EQ(align_index_up(vec[0], vec[1]), vec[3]);
  }
}
TEST(SDKTest, read_data_edge) {}
TEST(TestCommon, hash) {
  // md5sum, sha256sum
  vector<vector<string>> vec
      = {{"", "D41D8CD98F00B204E9800998ECF8427E",
          "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855"}};
  for (const auto& v : vec) {
    EXPECT_EQ(cal_md5sum(v[0]), v[1]);
    EXPECT_EQ(cal_sha256sum(v[0]), v[2]);
  }
}