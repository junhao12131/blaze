#pragma once

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

  template <class T2>
  std::vector<T2> mapreduce_dense(
      const std::function<
          void(const T value, const std::function<void(const int&, const T2&)>& emit)>& mapper,
      const std::function<void(T2&, const T2&)>& reducer,
      const int n_keys,
      const T2& default_value) {
    // Map and thread reduce.
    const int n_threads = omp_get_max_threads();
    std::vector<std::vector<T2>> res_threads(n_threads);
    for (int i = 0; i < n_threads; i++) {
      res_threads[i].assign(n_keys, default_value);
    }
    const auto& emit = [&](const int& key, const T2& value) {
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
        for (int j = 0; j < n_keys; j++) {
          reducer(res_threads[i][j], res_threads[i + step][j]);
        }
      }
      step <<= 1;
    }
    std::vector<T2> res_local = res_threads[0];

    // Cross-node tree reduce.
    std::vector<T2> res(n_keys, default_value);
    MPI_Allreduce(
        res_local.data(),
        res.data(),
        n_keys,
        internal::MpiType<T2>::value,
        MPI_SUM,
        MPI_COMM_WORLD);

    /*
        // Cross-node tree reduce.
        std::vector<std::vector<T2>> res_locals = gather(res_local);
        std::vector<T2> res = res_locals[0];
        const int n_procs = internal::MpiUtil::get_n_procs();
        const int proc_id = internal::MpiUtil::proc_id();
        int step = 2;
        while (step < n_procs) {
          if (proc_id % step == 0) {
          } else {
          }
        }
        std::vector<T2> res = broadcast()

    #pragma omp parallel for schedule(static, 1)
        for (int i = 1; i < n_procs; i++) {
          for (int j = 0; j < n_keys; j++) {
            reducer(res[j], res_locals[i][j]);
          }
        }
        */

    return res;
  }

  /*
    template <class K, class V, class H>
    void mapreduce(
        const std::function<void(const T value, const std::function<void(const K&, const V&)>&
    emit)>&
            mapper,
        const std::function<void(V&, const V&)>& reducer,
        DistHashMap<K, V, H>& dm) {
      const auto& emit = [&](const K& key, const V& value) {
        dm.async_set(key, value, reducer);
      };
      for_each([&](const T t) { mapper(t, emit); });
      dm.sync(reducer);
    }

    template <class T2>
    T2 mapreduce(
        const std::function<T2(const T value)>& mapper,
        const std::function<void(T2&, const T2&)>& reducer,
        const T2& default_value) {
      const int n_threads = omp_get_max_threads();
      std::vector<T2> res_thread(n_threads, default_value);
      for_each([&](const T t) {
        const int thread_id = omp_get_thread_num();
        reducer(res_thread[thread_id], mapper(t));
      });
      T2 res_local;
      T2 res;
      res_local = res_thread[0];
      for (int i = 1; i < n_threads; i++) res_local += res_thread[i];
      std::vector<T2> res_locals = gather(res_local);
      res = res_locals[0];
      const int n_procs = internal::MpiUtil::get_n_procs();
      for (int i = 1; i < n_procs; i++) reducer(res, res_locals[i]);
      return res;
    }

  */
 private:
  T start;

  T end;

  T inc;
};
}  // namespace blaze
