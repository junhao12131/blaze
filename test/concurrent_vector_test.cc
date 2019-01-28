#include "../src/internal/concurrent_vector.h"

#include <gtest/gtest.h>
#include <functional>

#include "../src/reducer.h"

TEST(ConcurrentVectorTest, SumSquares) {
  blaze::internal::ConcurrentVector<double> vec;
  const size_t LEN = 1000;
  vec.resize(LEN + 1, 0.0);

#pragma omp parallel for schedule(static, 1)
  for (size_t i = 1; i <= LEN; i++) {
    vec.set(i, i * i);
  }

  double result = 0.0;
  vec.for_each_serial([&](const size_t, const double value) { result += value; });
  const double expected = LEN * (LEN + 1) * (2 * LEN + 1) / 6;
  EXPECT_NEAR(result, expected, expected * 1.0e-10);
}

TEST(ConcurrentVectorTest, SumSquaresAsync) {
  blaze::internal::ConcurrentVector<double> vec;
  const size_t LEN = 1000;
  vec.resize(LEN + 1, 0.0);

#pragma omp parallel for schedule(static, 1)
  for (size_t i = 1; i <= LEN; i++) {
    vec.async_set(i, i * i, blaze::Reducer<double>::overwrite);
  }
  vec.sync();

  double result = 0.0;
  vec.for_each_serial([&](const size_t, const double value) { result += value; });
  const double expected = LEN * (LEN + 1) * (2 * LEN + 1) / 6;
  EXPECT_NEAR(result, expected, expected * 1.0e-10);
}
