#include "../src/collect.h"
#include "../src/distribute.h"

#include <gtest/gtest.h>
#include <vector>

#include "../src/dist_vector.h"

TEST(ConvertTest, IntegerVector) {
  std::vector<int> origin({0, 1, 2, 3, 4});
  blaze::DistVector<int> distributed = blaze::distribute(origin);
  std::vector<int> collected = blaze::collect(distributed);
  EXPECT_EQ(collected.size(), 5);
  EXPECT_EQ(collected[2], 2);
}
