#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>

#include "../../src/distribute.h"
#include "../../blaze.h"

TEST(BenchmarkTest, NearestNeighbor) {
  using namespace std::chrono;
  // std::ifstream file("test/benchmark/data/nn_data.txt");
  auto lines = blaze::util::load_file("test/benchmark/data/nn_data.txt");
  blaze::DistVector<std::array<int, 2>> dist_points(lines.size());

  const auto& mapper = [&](const size_t key, const std::string& line, const auto& emit) {
    std::stringstream ss(line);
    std::array<int, 2> x1;
    ss >> x1[0] >> x1[1];
    emit(key, x1);
  };
  blaze::mapreduce<std::string, std::array<int, 2>>(lines, mapper, blaze::Reducer<std::array<int, 2>>::overwrite, dist_points);


  /*
  if (!file.is_open()) throw std::runtime_error("Error openning file");

  std::array<int, 2> x1;
  size_t n = 0;
  blaze::DistVector<std::array<int, 2>> dist_points;
  while (true) {
    file >> x1[0];
    if (file.eof()) break;
    file >> x1[1];
    if (dist_points.is_local(n)) {
      dist_points.resize(n + 1);
      dist_points.async_set(n, x1);
    }
    n++;
  }
  dist_points.sync();
  */

  std::array<int, 2> x0;

  steady_clock::time_point start;
  for (int t = 0; t < 10; t++) {
    if (t == 1) {
      start = steady_clock::now();
    }
    x0[0] = t * t;
    x0[1] = t * t;
    auto it_start = steady_clock::now();
    auto res =
        dist_points.top_k(100, [&](const std::array<int, 2>& a, const std::array<int, 2>& b) {
          long long diff_a0 = a[0] - x0[0];
          long long diff_a1 = a[1] - x0[1];
          long long diff_b0 = b[0] - x0[0];
          long long diff_b1 = b[1] - x0[1];
          return diff_a0 * diff_a0 + diff_a1 * diff_a1 < diff_b0 * diff_b0 + diff_b1 * diff_b1;
        });
    auto it_end = steady_clock::now();
    printf("%d %d; %d %d\n", res[0][0], res[0][1], res[1][0], res[1][1]);
    std::cout << duration_cast<milliseconds>(it_end - it_start).count() << " ms\n";
  }
  auto end = steady_clock::now();
  std::cout << duration_cast<milliseconds>(end - start).count() << " ms\n";
}
