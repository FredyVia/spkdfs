#include <common/node.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace std;
using namespace spkdfs;

TEST(NodeTest, parse_nodes) {
  auto nodes = parse_nodes("127.0.0.1,127.0.0.2,127.0.0.3");
  for (auto &node : nodes) {
    cout << to_string(node) << endl;
  }
  // EXPECT_EQ(Endpoint::node_discovery(vec), 0);
}