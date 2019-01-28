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
  DistRange(const T start = 0, const T end = 0, const T inc = 1)
      : start(start), end(end), inc(inc) {}

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

  bool is_local(const T t) {
    const size_t n_procs_u = internal::MpiUtil::get_n_procs();
    const size_t proc_id_u = internal::MpiUtil::get_proc_id();
    return static_cast<size_t>((t - start) / inc) % n_procs_u == proc_id_u;
  }

 private:
  T start;

  T end;

  T inc;
};

}  // namespace blaze

#endif
