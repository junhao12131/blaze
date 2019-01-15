#include <gtest/gtest.h>
#include <chrono>
#include <functional>

#include "../../src/mapreduce.h"

template <size_t N>
void run_kmeans(
    const std::vector<std::array<double, N>>& points,
    std::vector<std::array<double, N>>& centers,
    double epsilon = 1.0e-10) {
  const size_t n_points = points.size();
  const size_t n_centers = centers.size();
  blaze::DistRange<size_t> range(0, n_points);

  const auto& mapper = [&](
      const size_t i, const std::function<void(const size_t&, const double&)>& emit) {
    const auto& point = points[i];
    size_t c = 0;
    double min_dist = std::numeric_limits<double>::max();
    for (size_t j = 0; j < n_centers; j++) {
      double dist = 0.0;
      for (size_t k = 0; k < N; k++) {
        double diff = point[k] - centers[j][k];
        dist += diff * diff;
      }
      if (dist < min_dist) {
        min_dist = dist;
        c = j;
      }
    }
    size_t id_base = (N + 1) * c;
    emit(id_base, 1.0);
    for (size_t k = 0; k < N; k++) {
      emit(id_base + k + 1, point[k]);
    }
  };

  double max_change = 1.0;
  std::vector<double> res;
  while (max_change > epsilon) {
    res.assign(n_centers * (N + 1), 0.0);
    blaze::mapreduce<size_t, double>(range, mapper, "sum", res);
    max_change = 0.0;
    for (size_t i = 0; i < n_centers; i++) {
      size_t id_base = i * (N + 1);
      double nc = res[id_base];
      double dist = 0.0;
      for (size_t k = 0; k < N; k++) {
        double ck = res[id_base + k + 1] / nc;
        double change = ck - centers[i][k];
        dist += change * change;
        centers[i][k] = ck;
      }
      max_change = std::max(max_change, dist);
    }
    max_change = std::sqrt(max_change);
  }
}

TEST(BenchmarkTest, KMeans) {
  std::ifstream file("test/benchmark/data/kmeans_data_small.txt");
  std::vector<std::array<double, 3>> points;
  std::array<double, 3> point;
  if (!file.is_open()) throw std::runtime_error("Error opening file");
  while (true) {
    for (int i = 0; i < 3; i++) {
      file >> point[i];
    }
    if (file.eof()) break;
    points.push_back(point);
  }

  const int n_centers = 5;
  std::vector<std::array<double, 3>> centers(n_centers);

  std::chrono::steady_clock::time_point start;
  for (int t = 0; t <= 5; t++) {
    if (t == 1) {
      start = std::chrono::steady_clock::now();
    }
    printf("Iteration %d:\n", t);
    for (int i = 0; i < n_centers; i++) {
      centers[i].fill(0);
      centers[i][0] = i;
    }
    run_kmeans(points, centers, 1.0e-10);
    for (int i = 0; i < n_centers; i++) {
      for (int k = 0; k < 3; k++) {
        printf("%.8f ", centers[i][k]);
      }
      printf("\n");
    }
  }
  auto end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}
