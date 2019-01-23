#ifndef BLAZE_DIST_RANGE_H_
#define BLAZE_DIST_RANGE_H_

#include <mpi.h>
#include <omp.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <vector>

// #include "dist_hash_map.h"
// #include "gather.h"
#include "internal/mpi_type.h"
#include "internal/mpi_util.h"
#include "reducer.h"

namespace blaze {

template <class T>
class DistRange {
 public:
  DistRange(const T start, const T end, const T inc = 1) : start(start), end(end), inc(inc) {}

  void for_each(const std::function<void(const T)>& handler, const bool verbose = false) {
    const int n_procs = internal::MpiUtil::get_n_procs();
    const int proc_id = internal::MpiUtil::get_proc_id();
    double target_progress = 0.1;
    T t_start = start + inc * proc_id;
    T t_step = inc * n_procs;
#pragma omp parallel for schedule(dynamic, 4)
    for (T t = t_start; t < end; t += t_step) {
      handler(t);
      const int thread_id = omp_get_thread_num();
      if (!verbose || thread_id != 0) continue;
      const double current_progress = static_cast<double>(t - start) / (end - start);
      while (target_progress < current_progress) {
        printf("%.0f%% ", target_progress * 100);
        target_progress += 0.1;
      }
    }
    if (verbose) {
      while (target_progress <= 1.0) {
        printf("%.0f%% ", target_progress * 100);
        target_progress += 0.1;
      }
      printf("\n");
    }
  }

  template <class V>
  void mapreduce(
      const std::function<
          void(const T value, const std::function<void(const size_t&, const V&)>& emit)>& mapper,
      const std::function<void(V&, const V&)>& reducer,
      std::vector<V>& target,
      const V& default_value) {
    // Map and thread reduce.
    const int n_threads = omp_get_max_threads();
    const size_t n_keys = target.size();
    std::vector<std::vector<V>> res_threads(n_threads);
    for (int i = 0; i < n_threads; i++) {
      res_threads[i].assign(n_keys, default_value);
    }
    const auto& emit = [&](const size_t key, const V& value) {
      const int thread_id = omp_get_thread_num();
      reducer(res_threads[thread_id][key], value);
    };
    for_each([&](const T t) { mapper(t, emit); });

    // Node reduce.
    int step = 1;
    while (step < n_threads) {
      int i_end = n_threads - step;
      int i_step = step << 1;
#pragma omp parallel for schedule(static, 1)
      for (int i = 0; i < i_end; i += i_step) {
        for (size_t j = 0; j < n_keys; j++) {
          reducer(res_threads[i][j], res_threads[i + step][j]);
        }
      }
      step <<= 1;
    }

    std::vector<V> res(n_keys);
    std::vector<V> res_local(n_keys);

#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n_keys; i++) {
      res_local[i] = res_threads[0][i];
    }

    // Cross-node tree reduce.
    MPI_Allreduce(
        res_local.data(),
        // res_threads[0].data(),
        res.data(),
        // res.data(),
        n_keys,
        internal::MpiType<V>::value,
        MPI_SUM,
        MPI_COMM_WORLD);

#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n_keys; i++) {
      reducer(target[i], res[i]);
    }
  }

 private:
  T start;

  T end;

  T inc;
};

}  // namespace blaze

#endif
