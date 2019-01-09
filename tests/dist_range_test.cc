#include "../src/dist_range.h"

#include <gtest/gtest.h>
#include <chrono>
#include <functional>

#include "../src/reducer.h"

TEST(DistRangeTest, MapreduceTest) {
  const int N_SAMPLES = 1000;
  blaze::DistRange<int> range(0, N_SAMPLES + 1);
  const auto& mapper = [&](const int i, const std::function<void(const int&, const int&)>& emit) {
    emit(0, i * i);
  };

  auto res = range.mapreduce_dense<int>(mapper, blaze::Reducer<int>::sum, 1, 0);
  EXPECT_EQ(res[0], 333833500);
}

template <size_t N>
void run_kmeans(
    const std::vector<std::array<double, N>>& points,
    std::vector<std::array<double, N>>& centers,
    double epsilon=1.0e-10) {
  const int n_points = points.size();
  const int n_centers = centers.size();
  blaze::DistRange<int> range(0, n_points);

  const auto& mapper = [&](
      const int i, const std::function<void(const int&, const double&)>& emit) {
    const auto& point = points[i];
    int c = -1;
    double min_dist = std::numeric_limits<double>::max();
    for (int j = 0; j < n_centers; j++) {
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
    int id_base = (N + 1) * c;
    emit(id_base, 1);
    for (size_t k = 0; k < N; k++) {
      emit(id_base + k + 1, point[k]);
    }
  };

  double max_change = 1.0;
  while (max_change > epsilon) {
    auto res = range.mapreduce_dense<double>(mapper, blaze::Reducer<double>::sum, n_centers * (N + 1), 0);
    max_change = 0.0;
    for (int i = 0; i < n_centers; i++) {
      int id_base = i * (N + 1);
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

TEST(DistRangeTest, MapreduceKMeansLargeTest) {
  std::ifstream file("tests/kmeans_data.txt");
  std::vector<std::array<double, 3>> points;
  std::array<double, 3> point;
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
  for (int t = 0; t <= 10; t++) {
    if (t == 1) {
      start = std::chrono::steady_clock::now();
    }
    printf("%d\n", t);
    for (int i = 0; i < n_centers; i++) {
      centers[i].fill(0);
      centers[i][0] = i;
    }
    run_kmeans(points, centers, 1.0e-10);
    for (int i = 0; i < n_centers; i++) {
      for (int k = 0; k < 3; k++) {
        printf("%.6f ", centers[i][k]);
      }
      printf("\n");
    }
  }
  auto end = std::chrono::steady_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
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
  const auto& mapper = [&](const int i, const std::function<void(const std::string&, const int&)>&
emit) {
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
