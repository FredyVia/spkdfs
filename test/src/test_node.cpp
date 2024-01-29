#include <common/node.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace std;
using namespace spkdfs;

TEST(NodeTest, node_discovery) {
  // std::vector<Endpoint> vec = {};
  // for (int i = 1; i < 20; i++) {
  //   vec.push_back(Endpoint("192.168.7." + to_string(i), 10086));
  // }
  // EXPECT_EQ(Endpoint::node_discovery(vec), 0);
}

TEST(NodeTest, parse_nodes) {
  parse_nodes("127.0.0.1,127.0.0.2,127.0.0.3");

  // EXPECT_EQ(Endpoint::node_discovery(vec), 0);
}