#include <gtest/gtest.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>

#include "../../src/mapreduce.h"

blaze::DistVector<double> get_page_rank(
    const size_t n,
    const std::unordered_map<size_t, std::vector<size_t>>& links,
    const double epsilon = 1.0e-5,
    const double d = 0.15) {
  const auto& sink_mapper = [&](
      const size_t key,
      const double& value,
      const std::function<void(const size_t, const double&)>& emit) {
    if (links.count(key) == 0) {
      emit(0, value);
    }
  };

  const auto& scatter_mapper = [&](
      const size_t key,
      const double& value,
      const std::function<void(const size_t, const double&)>& emit) {
    if (links.count(key) == 0) return;
    const auto& key_links = links.find(key)->second;
    size_t L = key_links.size();
    double factor = (1 - d) * value / L;
    for (size_t i = 0; i < L; i++) {
      emit(key_links[i], factor);
    }
  };

  const auto& diff_mapper = [&](
      const size_t,
      const double& value,
      const std::function<void(const size_t, const double&)>& emit) { emit(0, std::abs(value)); };

  blaze::DistVector<double> ranks(n, 1.0);
  std::vector<double> sink_sum(1, 0.0);
  std::vector<double> max_change(1, 1.0);
  size_t iteration = 0;
  while (max_change[0] > epsilon) {
    iteration++;
    sink_sum[0] = 0.0;
    blaze::mapreduce<double, double>(ranks, sink_mapper, "sum", sink_sum);

    blaze::DistVector<double> new_ranks(n, d + (1 - d) * sink_sum[0] / n);
    blaze::mapreduce<double, double>(ranks, scatter_mapper, blaze::Reducer<double>::sum, new_ranks);

    ranks -= new_ranks;
    max_change[0] = 0.0;
    blaze::mapreduce<double, double>(ranks, diff_mapper, "max", max_change);

    ranks = new_ranks;
  }

  return ranks;
}

TEST(BenchmarkTest, Pagerank) {
  std::ifstream file("test/benchmark/data/pagerank_data.txt");
  std::unordered_map<size_t, std::vector<size_t>> links;

  if (!file.is_open()) throw std::runtime_error("Error openning file");

  size_t origin;
  size_t dest;
  size_t n = 0;
  while (true) {
    file >> origin;
    if (file.eof()) break;
    file >> dest;

    if (origin >= n) n = origin + 1;
    if (dest >= n) n = dest + 1;

    if (!blaze::DistVector<double>::is_local(origin)) continue;
    links[origin].push_back(dest);
  }

  std::chrono::steady_clock::time_point start;

  for (int t = 0; t <= 3; t++) {
    if (t == 1) {
      start = std::chrono::steady_clock::now();
    }

    auto s0 = std::chrono::steady_clock::now();
    auto ranks = get_page_rank(n, links);
    auto s1 = std::chrono::steady_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(s1 - s0).count() << "ms\n";
    ranks.for_each([&](const size_t key, const double& value) {
      if (key % 200000 == 1) {
        printf("%zu: %.8f\n", key, value);
      }
    });
  }

  auto end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
}
