#include "../src/gather.h"

#include <gtest/gtest.h>
#include <string>
#include <unordered_map>

TEST(GatherTest, Integer) {
  int a = blaze::internal::MpiUtil::get_proc_id();
  const std::vector<int> res = blaze::gather(a);
  const int n_procs = blaze::internal::MpiUtil::get_n_procs();
  for (int i = 0; i < n_procs; i++) {
    EXPECT_EQ(res[i], i);
  }
}

TEST(GatherTest, UnorderedMap) {
  std::unordered_map<std::string, int> a;
  a["proc_id"] = blaze::internal::MpiUtil::get_proc_id();
  const auto& res = blaze::gather(a);
  const int n_procs = blaze::internal::MpiUtil::get_n_procs();
  for (int i = 0; i < n_procs; i++) {
    EXPECT_EQ(res[i].size(), 1);
    EXPECT_EQ(res[i].find("proc_id")->second, i);
  }
}
