#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "common/inode.h"
#include "common/utils.h"

using namespace std;
using namespace spkdfs;

TEST(ClientTest, encode_decode) {
  string file_path = "/tmp/spkdfs_test.bin";
  ofstream ofile(file_path, ios::binary);
  if (!ofile) {
    cerr << "Failed to open ofile for writing.";
    throw runtime_error("openfile error:" + file_path);
  }

  int size = 1;  // 8 * MB
  for (int i = 0; i <= size * 1024 * 1024; i++) {
    ofile << i << endl;
  }
  ofile.close();
  ifstream ifile(file_path, ios::binary);
  if (!ifile) {
    cerr << "Failed to open ofile for writing.";
    throw runtime_error("openfile error:" + file_path);
  }

  string pattern = "RS<3,2,64>";
  shared_ptr<StorageType> storage_type_ptr = StorageType::from_string(pattern);
  string block;
  cout << " using block size: " << storage_type_ptr->get_b() << endl;
  block.resize(storage_type_ptr->get_b());
  vector<string> res;
  while (ifile) {
    ifile.read(&block[0], storage_type_ptr->get_b());
    size_t bytesRead = ifile.gcount();
    cout << "bytesRead: " << bytesRead << endl;
    auto vec = storage_type_ptr->encode(block);
    EXPECT_TRUE(vec.size() > 0);
    // 上传每个编码后的分块
    for (auto& encodedData : vec) {
      cout << "encodedData.size(): " << encodedData.size() << endl;
      string sha256sum = cal_sha256sum(encodedData);
      cout << "sha256sum:" << sha256sum << endl;
      // 假设每个分块的大小是block.size() / k
      string filename = "/tmp/blk_" + sha256sum;
      ofstream bfile(filename, std::ios::out | ios::binary);
      if (!bfile) {
        cerr << "Failed to open ofile for writing.";
        throw runtime_error("openfile error:" + file_path);
      }
      bfile << encodedData;
      bfile.close();
      res.push_back(filename);
    }
  }
  // EXPECT_EQ(Endpoint::node_discovery(vec), 0);
}