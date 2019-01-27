#include <gtest/gtest.h>
#include <chrono>
#include <cmath>
#include <functional>

#include "../../src/mapreduce.h"
#include "../../vendor/eigen-git-mirror/Eigen/Dense"

template <size_t N>
void run_em_gaussian_mixture(
    const std::vector<std::array<double, N>>& points,
    std::vector<double>& weights,
    std::vector<std::array<double, N>>& centers,
    std::vector<std::array<std::array<double, N>, N>>& sigmas,
    double epsilon = 1.0e-10,
    size_t max_iterations = 100) {
  using namespace std::chrono;
  const size_t n_points = points.size();
  const size_t n_centers = centers.size();

  auto n_points_range = blaze::DistRange<size_t>(0, n_points);
  blaze::DistVector<std::vector<double>> likelihood(n_points);
  blaze::DistVector<std::vector<double>> membership(n_points);
  std::vector<double> sum_weights(n_centers);

  const double PI = 3.14159265358979323846;
  std::vector<double> denominators(n_centers);
  std::vector<Eigen::Matrix<double, N, N>> eigen_inv_sigmas(n_centers);
  std::vector<Eigen::Matrix<double, N, 1>> eigen_centers(n_centers);
  for (size_t i = 0; i < n_centers; i++) {
    Eigen::Matrix<double, N, N> eigen_sigma;
    Eigen::Matrix<double, N, 1> eigen_center;
    for (size_t j = 0; j < N; j++) {
      for (size_t k = 0; k < N; k++) {
        eigen_sigma(j, k) = sigmas[i][j][k];
      }
      eigen_center[j] = centers[i][j];
    }
    eigen_inv_sigmas[i] = eigen_sigma.inverse();
    eigen_centers[i] = eigen_center;
    denominators[i] = 1.0 / (std::pow(2 * PI, 0.5 * N) * std::sqrt(eigen_sigma.determinant()));
  }

  const auto& likelihood_mapper = [&](
      const size_t i, const std::function<void(const size_t, const std::vector<double>&)>& emit) {
    Eigen::Matrix<double, N, 1> x;
    for (size_t j = 0; j < N; j++) x[j] = points[i][j];
    std::vector<double> point_likelihood(n_centers);
    for (size_t j = 0; j < n_centers; j++) {
      Eigen::Matrix<double, N, 1> diff = x - eigen_centers[j];
      const double exponent = -0.5 * (diff.transpose() * eigen_inv_sigmas[j] * diff).value();
      point_likelihood[j] = denominators[j] * std::exp(exponent);
    }
    emit(i, point_likelihood);
  };

  const auto& membership_mapper = [&](
      const size_t key,
      const std::vector<double>& value,
      const std::function<void(const size_t, const std::vector<double>&)>& emit) {
    double scale = 0.0;
    std::vector<double> point_membership(n_centers);
    for (size_t i = 0; i < n_centers; i++) {
      double vw = value[i] * weights[i];
      scale += vw;
      point_membership[i] = vw;
    }
    scale = 1.0 / scale;
    for (size_t i = 0; i < n_centers; i++) {
      point_membership[i] *= scale;
    }
    emit(key, point_membership);
  };

  const auto& log_likelihood_mapper = [&](
      const size_t,
      const std::vector<double>& value,
      const std::function<void(const size_t, const double&)>& emit) {
    double log_likelihood = 0.0;
    for (size_t i = 0; i < n_centers; i++) {
      log_likelihood += value[i] * weights[i];
    }
    log_likelihood = log(log_likelihood);
    emit(0, log_likelihood);
  };

  const auto& sum_weights_mapper = [&](
      const size_t,
      const std::vector<double>& value,
      const std::function<void(const size_t, const double&)>& emit) {
    for (size_t i = 0; i < n_centers; i++) {
      emit(i, value[i]);
    }
  };

  const auto& centers_mapper = [&](
      const size_t key,
      const std::vector<double>& value,
      const std::function<void(const size_t, const double&)>& emit) {
    size_t flat_key = 0;
    for (size_t i = 0; i < n_centers; i++) {
      double elem_base = value[i];
      for (size_t j = 0; j < N; j++) {
        emit(flat_key, elem_base * points[key][j]);
        flat_key++;
      }
    }
  };

  const auto& sigma_mapper = [&](
      const size_t key,
      const std::vector<double>& value,
      const std::function<void(const size_t, const double&)>& emit) {
    size_t flat_key = 0;
    for (size_t i = 0; i < n_centers; i++) {
      for (size_t j = 0; j < N; j++) {
        double elem_base = value[i] * (points[key][j] - centers[i][j]);
        for (size_t k = 0; k < N; k++) {
          double elem = elem_base * (points[key][k] - centers[i][k]);
          emit(flat_key, elem);
          flat_key++;
        }
      }
    }
  };

  double likelihood_change = 1.0;  // max log likelihood change.
  std::vector<double> log_likelihood_vec(1, 0.0);
  std::vector<double> prev_log_likelihood_vec(1, 0.0);
  std::vector<double> flat_centers;
  std::vector<double> flat_sigmas;

  size_t iteration = 0;
  while (likelihood_change > epsilon) {
    iteration++;
    if (iteration > max_iterations) break;

    // Expectation.
    blaze::mapreduce<size_t, std::vector<double>>(
        n_points_range,
        likelihood_mapper,
        blaze::Reducer<std::vector<double>>::overwrite,
        likelihood);
    blaze::mapreduce<std::vector<double>, std::vector<double>>(
        likelihood, membership_mapper, blaze::Reducer<std::vector<double>>::overwrite, membership);

    // Update criteria.
    log_likelihood_vec[0] = 0.0;
    blaze::mapreduce<std::vector<double>, double>(
        likelihood, log_likelihood_mapper, "sum", log_likelihood_vec);
    likelihood_change = std::abs(log_likelihood_vec[0] - prev_log_likelihood_vec[0]);
    prev_log_likelihood_vec = log_likelihood_vec;

    // Maximization.
    sum_weights.assign(n_centers, 0.0);
    blaze::mapreduce<std::vector<double>, double>(
        membership, sum_weights_mapper, "sum", sum_weights);

    for (size_t i = 0; i < n_centers; i++) {
      weights[i] = sum_weights[i] / n_points;

      centers[i].fill(0.0);
      for (size_t j = 0; j < N; j++) {
        sigmas[i][j].fill(0.0);
      }
    }

    flat_centers.assign(n_centers * N, 0.0);
    blaze::mapreduce<std::vector<double>, double>(membership, centers_mapper, "sum", flat_centers);
    for (size_t i = 0; i < n_centers; i++) {
      for (size_t j = 0; j < N; j++) {
        centers[i][j] = flat_centers[i * N + j] / sum_weights[i];
      }
    }
    flat_sigmas.assign(n_centers * N * N, 0.0);
    blaze::mapreduce<std::vector<double>, double>(membership, sigma_mapper, "sum", flat_sigmas);

#pragma omp parallel for schedule(static, 1)
    for (size_t i = 0; i < n_centers; i++) {
      for (size_t j = 0; j < N; j++) {
        for (size_t k = 0; k < N; k++) {
          sigmas[i][j][k] = flat_sigmas[i * N * N + j * N + k] / sum_weights[i];
        }
      }
    }

    for (size_t i = 0; i < n_centers; i++) {
      Eigen::Matrix<double, N, 1> eigen_center;
      Eigen::Matrix<double, N, N> eigen_sigma;
      for (size_t j = 0; j < N; j++) {
        eigen_center[j] = centers[i][j];
        for (size_t k = 0; k < N; k++) {
          eigen_sigma(j, k) = sigmas[i][j][k];
        }
      }
      eigen_inv_sigmas[i] = eigen_sigma.inverse();
      eigen_centers[i] = eigen_center;
      denominators[i] = 1.0 / (std::pow(2 * PI, 0.5 * N) * std::sqrt(eigen_sigma.determinant()));
    }
  }
}

TEST(BenchmarkTest, EMGaussianMixture) {
  std::ifstream file("test/benchmark/data/cluster_data_em.txt");
  std::vector<std::array<double, 3>> points;
  std::array<double, 3> point;
  using namespace std::chrono;

  if (!file.is_open()) throw std::runtime_error("Error opening file");
  while (true) {
    for (int i = 0; i < 3; i++) {
      file >> point[i];
    }
    if (file.eof()) break;
    points.push_back(point);
  }

  const int n_centers = 5;
  std::vector<double> weights(n_centers);
  std::vector<std::array<double, 3>> centers(n_centers);
  std::vector<std::array<std::array<double, 3>, 3>> sigmas(n_centers);

  steady_clock::time_point start;
  for (int t = 0; t <= 3; t++) {
    if (t == 1) {
      start = steady_clock::now();
    }
    printf("Iteration %d:\n", t);
    weights.assign(n_centers, 1.0);
    for (int i = 0; i < n_centers; i++) {
      centers[i].fill(0);
      centers[i][0] = i;
      for (int j = 0; j < 3; j++) {
        sigmas[i][j].fill(0.0);
        sigmas[i][j][j] = 1.0;
      }
    }
    auto it_start = steady_clock::now();
    run_em_gaussian_mixture(points, weights, centers, sigmas, 1.0e-2);
    auto it_end = steady_clock::now();
    for (int i = 0; i < n_centers; i++) {
      for (int k = 0; k < 3; k++) {
        printf("%.8f ", centers[i][k]);
      }
      printf("\n");
    }
    std::cout << duration_cast<milliseconds>(it_end - it_start).count() << " ms\n";
  }
  auto end = steady_clock::now();
  std::cout << duration_cast<milliseconds>(end - start).count() << " ms\n";
}
