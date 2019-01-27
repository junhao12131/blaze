#include "../src/dist_hash_map.h"

#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <iostream>
#include "../src/dist_range.h"

TEST(DistHashMapTest, AsyncSetAndSyncTest) {
  const long long N_KEYS = 100;
  blaze::DistHashMap<long long, long long> ds;
  blaze::DistRange<long long> range(0, N_KEYS);
  long long sum = 0;
  range.for_each([&](const long long i) { ds.async_set(i, i); });
  ds.sync();
  ds.for_each_serial([&](const long long i, const size_t, const long long) { sum += i; });
  EXPECT_EQ(sum, N_KEYS * (N_KEYS - 1) / 2);
}

TEST(DistHashMapTest, Mapreduce) {
  const long long N_KEYS = 100;
  blaze::DistHashMap<long long, long long> ds;
  blaze::DistRange<long long> range(0, N_KEYS);
  range.for_each([&](const long long i) { ds.async_set(i, i); });
  ds.sync();
  long long sum = ds.mapreduce<long long>(
      [&](const long long key, const long long) { return key; }, blaze::Reducer<long long>::sum, 0);
  EXPECT_EQ(sum, N_KEYS * (N_KEYS - 1) / 2);
}

