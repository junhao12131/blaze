#include "../src/dist_range.h"

#include <gtest/gtest.h>
#include <functional>

#include "../src/reducer.h"

TEST(DistRangeTest, MapreduceTest) {
  const int N_SAMPLES = 10000000;
  blaze::DistRange<int> range(0, N_SAMPLES);
  const auto& mapper = [&](const int, const std::function<void(const int&, const int&)>& emit) {
    double x = double(rand()) / RAND_MAX;
    double y = double(rand()) / RAND_MAX;
    if (x * x + y * y < 1) {
      emit(0, 1);
    }
  };

  auto res = range.mapreduce_dense<int>(mapper, blaze::Reducer<int>::sum, 1, 0);
  std::cout << res[0] << std::endl;
  EXPECT_NEAR(4.0 * res[0] / N_SAMPLES, 3.1416, 0.1);
}

/*
TEST(DistRangeTest, ForEachTest) {
  const long long N_KEYS = 1000;
  blaze::DistRange<long long> range(0, N_KEYS);
  long long sum_local = 0;
  const auto& handler = [&](const long long i) {
    EXPECT_GE(i, 0);
    EXPECT_LT(i, N_KEYS);
#pragma omp atomic
    sum_local += i;
  };
  range.for_each(handler);
  long long sum = 0;
  MPI_Allreduce(&sum_local, &sum, 1, MPI_LONG_LONG_INT, MPI_SUM, MPI_COMM_WORLD);
  EXPECT_EQ(sum, N_KEYS * (N_KEYS - 1) / 2);
}

TEST(DistRangeTest, MapreduceTestToMap) {
  std::ifstream file("/home/junhao/Downloads/bible+shakes.nopunc");
  std::vector<std::string> lines;
  lines.reserve(1e8);
  std::string line;
  while (std::getline(file, line)) {
    lines.push_back(line);
  }
  std::cout << lines.size() << std::endl;
  blaze::DistRange<int> range(0, lines.size());
  blaze::DistHashMap<std::string, int> target;
  const auto& mapper = [&](const int i, const std::function<void(const std::string&, const int&)>& emit) {
    // const auto& mapper = [&](const int i, const auto& emit) {
    std::stringstream ss(lines[i]);
    std::string word;
    while (std::getline(ss, word, ' ')) {
      emit(word, 1);
    }
  };
  range.mapreduce<std::string, int, std::hash<std::string>>(
      mapper, blaze::Reducer<int>::sum, target);
  std::cout << target.get_n_keys() << std::endl;
}

TEST(DistRangeTest, MapreduceTest) {
  const int N_SAMPLES = 1000000;
  blaze::DistRange<int> range(0, N_SAMPLES);
  const auto& mapper = [&](const int) {
    double x = double(rand()) / RAND_MAX;
    double y = double(rand()) / RAND_MAX;
    if (x * x + y * y < 1) return 1;
    return 0;
  };
  int cnt = range.mapreduce<int>(mapper, blaze::Reducer<int>::sum, 0);
  EXPECT_NEAR(4.0 * cnt / N_SAMPLES, 3.1416, 0.1);
}
*/
