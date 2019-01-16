#include "../src/dist_vector_mapreducer.h"

#include <gtest/gtest.h>
#include <functional>

#include "../src/dist_range.h"
#include "../src/reducer.h"

TEST(DistVectorTest, SumSquares) {
  blaze::DistVector<size_t> vec;
  const size_t LEN = 1 << 10;
  vec.resize(LEN + 1);

  blaze::DistVector<size_t> vec2;
  vec2.resize(LEN + 1);
  const auto& mapper = [&](
      const size_t key,
      const size_t&,
      const std::function<void(const size_t, const size_t& value)>& emit) {
    emit((key + 3) % LEN, key * key);
  };
  blaze::DistVectorMapreducer<size_t>::mapreduce<size_t>(
      vec, mapper, blaze::Reducer<size_t>::overwrite, vec2);

  std::vector<size_t> res(1, 0);
  const auto& mapper2 = [&](
      const size_t,
      const size_t& value,
      const std::function<void(const size_t, const size_t& value)>& emit) { emit(0, value); };

  blaze::DistVectorMapreducer<size_t>::mapreduce<size_t>(vec2, mapper2, "sum", res);

  const size_t expected = LEN * (LEN + 1) * (2 * LEN + 1) / 6;
  EXPECT_EQ(res[0], expected);
}

TEST(DistVectorTest, TopK) {
  const size_t LEN = (1 << 10) + 15;
  blaze::DistVector<double> vec(LEN);
  vec.for_each([&](const size_t key, double& value) { value = -1.0 * key * key + 5 * key; });
  // values: 4, 6, 6, 4, 0, ...

  auto res = vec.top_k(3, [&](const double& a, const double& b) { return a > b; });
  EXPECT_EQ(res[0], 6);
  EXPECT_EQ(res[1], 6);
  EXPECT_EQ(res[2], 4);
}
