#include <common/inode.h>
#include <common/node.h>
#include <gtest/gtest.h>

#include <random>
#include <string>
#include <vector>

using namespace std;
using namespace spkdfs;

TEST(NodeTest, parse_nodes) {
  vector<vector<pair<string, int>>> ipports_group = {
      {{"127.0.0.1", 10086}, {"127.0.0.2", 10086}, {"127.0.0.3", 10086}}, {{{"127.0.0.1", 10086}}}};
  for (auto &ipports : ipports_group) {
    vector<string> res(4);  // test ip,ip | ip:port,ip:port | ip,ip, | ip:port,ip:port,
    for (auto &p : ipports) {
      res[0] += p.first + ",";
      res[1] += p.first + ",";
      res[2] += p.first + ":" + to_string(p.second) + ",";
      res[3] += p.first + ":" + to_string(p.second) + ",";
    }
    res[0].pop_back();
    res[2].pop_back();

    for (int i = 0; i < res.size(); i++) {
      std::vector<Node> nodes = parse_nodes(res[i]);
      EXPECT_EQ(nodes.size(), ipports.size());
      for (int j = 0; j < nodes.size(); j++) {
        EXPECT_EQ(nodes[j].ip, ipports[j].first);
        if (i < 2) {
          EXPECT_EQ(nodes[j].port, 0);
        } else {
          EXPECT_EQ(nodes[j].port, ipports[j].second);
        }
      }
    }
  }
}

TEST(InodeTest, RSStorageType) {
  vector<pair<string, vector<int>>> res = {
      {"RS<1,2,3>", {1, 2, 3}},
      {"RS<888,88,8>", {888, 88, 8}},
      {"RS<12,21,39>", {12, 21, 39}},
  };
  for (auto &p : res) {
    RSStorageType *rs_storage_type_ptr
        = dynamic_cast<RSStorageType *>(StorageType::from_string(p.first).get());
    EXPECT_EQ(rs_storage_type_ptr->k, p.second[0]);
    EXPECT_EQ(rs_storage_type_ptr->m, p.second[1]);
    EXPECT_EQ(rs_storage_type_ptr->getBlockSize() / 1024 / 1024, p.second[2]);
  }
}

TEST(InodeTest, REStorageType) {
  vector<pair<string, vector<int>>> res = {
      {"RE<2,3>", {2, 3}},
      {"RE<88,8>", {88, 8}},
      {"RE<21,39>", {21, 39}},
  };
  for (auto &p : res) {
    REStorageType *re_storage_type_ptr
        = dynamic_cast<REStorageType *>(StorageType::from_string(p.first).get());
    EXPECT_EQ(re_storage_type_ptr->replications, p.second[0]);
    EXPECT_EQ(re_storage_type_ptr->getBlockSize() / 1024 / 1024, p.second[1]);
  }
}

TEST(InodeTest, inode_parent_path) {
  vector<vector<string>> datas = {
      {"/tests", "/"},           {"/tests/", "/"}, {"/tests/abc", "/tests"},
      {"/tests/abc/", "/tests"}, {"//", "/"},      {"/", "/"},
  };
  Inode inode;
  for (auto &data : datas) {
    inode.set_fullpath(data[0]);
    EXPECT_EQ(inode.parent_path(), data[1])
        << "data[0]: " << data[0] << ", data[1]: " << data[1]
        << " ,inode.get_fullpath(): " << inode.get_fullpath() << ";";
  }
}

TEST(ClientTest, RS_encode_decode) {
  vector<string> datas = {"12345", "qwer", "", "\0"};

  // fatal "RS<12,8,16>", "RS<12,8,64>" "RS<12,8,256>",
  vector<string> storage_types
      = {"RS<3,2,16>", "RS<3,2,64>", "RS<3,2,256>", "RS<7,5,16>", "RS<7,5,64>", "RS<7,5,256>"};
  srand(time(0));
  auto generateBlock = [](size_t length) {
    std::random_device rd;   // 用于获取随机数种子
    std::mt19937 gen(rd());  // 标准 mersenne_twister_engine
    std::uniform_int_distribution<> distrib(0, 9);

    std::string block;
    for (size_t i = 0; i < length; ++i) {
      block += std::to_string(distrib(gen));
    }

    return block;
  };
  for (auto &storage_type : storage_types) {
    auto rs_ptr = StorageType::from_string(storage_type);
    for (auto &data : datas) {
      try {
        string res = rs_ptr->decode(rs_ptr->encode(data));
        EXPECT_EQ(res, data);
      } catch (const exception &e) {
        FAIL() << e.what() << ", storage_type" << storage_type << endl;
      }
    }
    // max: data_size = 2.4 * blocksize
    string data = generateBlock(((rand() % 60) * 1.0 / 40) * rs_ptr->getBlockSize());
    cout << "generateBlock size: " << data.size() << endl;
    EXPECT_EQ(rs_ptr->decode(rs_ptr->encode(data)), data);
  }
}

TEST(InodeTest, full_path) {}

TEST(InodeTest, from_json) {}

TEST(InodeTest, to_json) {}
TEST(InodeTest, stl) {
  try {
    string s;
    s.pop_back();
  } catch (const exception &e) {
    FAIL() << "We shouldn't get here.";
  }
  try {
    try {
      throw 20;
    } catch (int e) {
      throw string("abc");
    } catch (std::string const &ex) {
      FAIL() << "re throw We shouldn't get here.";
    }
  } catch (std::string const &ex) {
    SUCCEED() << "re throw go here";
  }
}