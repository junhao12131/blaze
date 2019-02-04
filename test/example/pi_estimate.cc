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

TEST(ExampleTest, PiEstimateRawWarm) {
  const size_t N_SAMPLES = 1000000;
  int n_procs;
  int proc_id;
  MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);

  double sum_local = 0.0;
  size_t start = proc_id;
  std::vector<double> res_threads(omp_get_max_threads());
#pragma omp parallel for
  for (size_t i = start; i < N_SAMPLES; i += n_procs) {
    double x = thread_safe_rand();
    double y = thread_safe_rand();
    if (x * x + y * y < 1) {
      res_threads[omp_get_thread_num()]++;
    }
  }
  for (int i = 0; i < omp_get_max_threads(); i++) sum_local += res_threads[i];

  double sum_global;
  MPI_Allreduce(&sum_local, &sum_global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  std::cout << 4.0 * sum_global / N_SAMPLES << std::endl;
}

TEST(ExampleTest, PiEstimateRaw) {
  const size_t N_SAMPLES = 1000000;
  int n_procs;
  int proc_id;
  MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);

  double sum_local = 0.0;
  size_t start = proc_id;
  std::vector<double> res_threads(omp_get_max_threads());
#pragma omp parallel for
  for (size_t i = start; i < N_SAMPLES; i += n_procs) {
    double x = thread_safe_rand();
    double y = thread_safe_rand();
    if (x * x + y * y < 1) {
      res_threads[omp_get_thread_num()]++;
    }
  }
  for (int i = 0; i < omp_get_max_threads(); i++) sum_local += res_threads[i];

  double sum_global;
  MPI_Allreduce(&sum_local, &sum_global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  std::cout << 4.0 * sum_global / N_SAMPLES << std::endl;
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
