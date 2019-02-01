#include <gtest/gtest.h>

#include <chrono>
#include <random>

#include "../../blaze.h"

double thread_safe_rand() {
  static thread_local std::mt19937 generator(
      std::chrono::system_clock::now().time_since_epoch().count());
  std::uniform_real_distribution<double> distribution;
  return distribution(generator);
}

TEST(ExampleTest, PiEstimate) {
  const size_t N_SAMPLES = 1000000;
  blaze::DistRange<size_t> samples(0, N_SAMPLES);
  const auto& mapper = [&](const size_t, const auto& emit) {
    double x = thread_safe_rand();
    double y = thread_safe_rand();
    if (x * x + y * y < 1) emit(0, 1);
  };
  std::vector<size_t> count(1);
  blaze::mapreduce<size_t, size_t>(samples, mapper, "sum", count);
  std::cout << 4.0 * count[0] / N_SAMPLES << std::endl;
}
