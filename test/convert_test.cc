#include "../src/collect.h"
#include "../src/distribute.h"

#include <gtest/gtest.h>

TEST(ConvertTest, IntegerVector) {
  std::vector<int> origin({0, 1, 2, 3, 4});
  blaze::DistVector<int> distributed = blaze::distribute(origin);
  std::vector<int> collected = blaze::collect(distributed);
  EXPECT_EQ(collected.size(), 5);
  EXPECT_EQ(collected[2], 2);
}

TEST(ConvertTest, IntegerMap) {
  std::unordered_map<int, int> origin({{0, 0}, {1, 1}, {2, 4}, {3, 9}, {4, 16}});
  blaze::DistHashMap<int, int> distributed = blaze::distribute(origin);
  EXPECT_EQ(distributed.get_n_keys(), 5);
  std::unordered_map<int, int> collected = blaze::collect(distributed);
  EXPECT_EQ(collected.size(), 5);
  EXPECT_EQ(collected[2], 4);
}
