#include "../src/dist_range_mapreducer.h"

#include <gtest/gtest.h>
#include <chrono>
#include <functional>

#include "../src/reducer.h"

TEST(DistRangeTest, SumSquaresMapreduce) {
  const size_t N_SAMPLES = 1000;
  blaze::DistRange<size_t> range(1, N_SAMPLES + 1);
  const auto& mapper = [&](
      const size_t i, const std::function<void(const size_t&, const size_t&)>& emit) {
    emit(0, i * i);
  };

  std::vector<size_t> result(1);
  blaze::DistRangeMapreducer<size_t>::mapreduce<size_t>(range, mapper, "sum", result);
  const size_t expected = N_SAMPLES * (N_SAMPLES + 1) * (2 * N_SAMPLES + 1) / 6;
  EXPECT_EQ(result[0], expected);
}
