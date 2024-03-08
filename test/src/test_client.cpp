#include <brpc/channel.h>
#include <brpc/controller.h>  // brpc::Controller
#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "common/inode.h"
#include "common/utils.h"
#include "service.pb.h"

using namespace std;
using namespace spkdfs;
using namespace brpc;

TEST(ClientTest, encode_decode) {}
TEST(ClientTest, concurrency) {
  // string namenode, namenode_master;
  // Channel dn_channel;
  // if (dn_channel.Init(string("192.168.88.108:8001").c_str(), NULL) != 0) {
  //   throw runtime_error("datanode channel init failed");
  // }
  // DatanodeService_Stub dn_stub(&dn_channel);

  // Controller cntl;
  // Request request;
  // DNGetNamenodesResponse response;
  // dn_stub.get_namenodes(&cntl, &request, &response, NULL);

  // if (cntl.Failed()) {
  //   cerr << "RPC failed: " << cntl.ErrorText();
  //   throw runtime_error("rpc get_namenodes failed");
  // }
  // if (!response.common().success()) {
  //   throw runtime_error(response.common().fail_info());
  // }
  // cout << "namenodes: ";
  // for (auto node : response.nodes()) {
  //   cout << node << ", ";
  // }
  // cout << endl;
  // cout << "using namenode: " << response.nodes(0) << " to probe namenode master" << endl;
  // namenode = response.nodes(0);
  // Channel nn_channel;
  // if (nn_channel.Init(string(namenode).c_str(), NULL) != 0) {
  //   throw runtime_error("get_master channel init failed");
  // }

  // NamenodeService_Stub nn_stub(&nn_channel);

  // NNGetMasterResponse master_response;
  // cntl.Reset();
  // nn_stub.get_master(&cntl, &request, &master_response, NULL);

  // if (cntl.Failed()) {
  //   cerr << "RPC failed: " << cntl.ErrorText();
  //   throw runtime_error("rpc get_master failed");
  // }
  // if (!master_response.common().success()) {
  //   throw runtime_error(master_response.common().fail_info());
  // }
  // cout << "namenode master: " << master_response.node() << endl;
  // namenode_master = master_response.node();

  // vector<std::thread> threads;

  // // 创建并启动多个线程
  // for (int i = 0; i < 5; ++i) {
  //   threads.emplace_back([i, &namenode_master]() {
  //     // 每个线程打印其标识符
  //     Controller cntl;
  //     NNPathRequest request;
  //     CommonResponse response;
  //     *(request.mutable_path()) = "/test";
  //     Channel nn_master_channel;
  //     if (nn_master_channel.Init(string(namenode_master).c_str(), NULL) != 0) {
  //       throw runtime_error("namenode_master channel init failed");
  //     }
  //     NamenodeService_Stub nn_master_stub(&nn_master_channel);

  //     nn_master_stub.mkdir(&cntl, &request, &response, NULL);
  //     if (cntl.Failed()) {
  //       cerr << "RPC failed: " << cntl.ErrorText();
  //       throw runtime_error("RPC failed: " + cntl.ErrorText());
  //     }
  //     if (!response.common().success()) {
  //       cerr << "failed" << response.common().fail_info() << endl;
  //       throw runtime_error(response.common().fail_info());
  //     }
  //     cout << "success" << endl;
  //   });
  // }

  // // 等待所有线程完成
  // for (auto& t : threads) {
  //   t.join();
  // }
}
TEST(TestCommon, alignup) {
  vector<vector<int>> checks = {{2, 5, 0, 1}, {4, 5, 0, 1}, {5, 5, 1, 2}, {6, 5, 1, 2},
                                {1, 2, 0, 1}, {2, 2, 1, 2}, {4, 2, 2, 3}, {5, 2, 2, 3}};
  for (auto& vec : checks) {
    EXPECT_EQ(align_index_down(vec[0], vec[1]), vec[2]);
    EXPECT_EQ(align_index_up(vec[0], vec[1]), vec[3]);
  }
}
TEST(TestCommon, rethrow) {
  try {
    try {
      throw 20;
    } catch (int e) {
      cout << "An exception occurred. Exception Nr. " << e << endl;
      throw string("abc");
    } catch (std::string const& ex) {
      EXPECT_EQ(1, 2);
      cout << "first string " << ex << endl;
    }
  } catch (std::string const& ex) {
    EXPECT_EQ(1, 1);
    cout << "second string " << ex << endl;
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