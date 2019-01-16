#pragma once

#include <mpi.h>
#include <omp.h>
#include <cstring>
#include <functional>
#include <vector>

#include "dist_range.h"
#include "internal/mapreduce_util.h"
#include "internal/mpi_type.h"
#include "internal/mpi_util.h"
#include "reducer.h"

namespace blaze {

template <class VS>
class DistRangeMapreducer {
 public:
  template <class VD>
  static void mapreduce(
      blaze::DistRange<VS>& source,
      const std::function<
          void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::string& reducer,
      std::vector<VD>& dest);
};

template <class VS>
template <class VD>
void DistRangeMapreducer<VS>::mapreduce(
    blaze::DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::string& reducer,
    std::vector<VD>& dest) {
  // Map and thread reduce.
  const int n_threads = omp_get_max_threads();
  const size_t n_keys = dest.size();
  std::vector<std::vector<VD>> res_threads(n_threads);
  const VD default_value = VD();
  for (int i = 0; i < n_threads; i++) {
    res_threads[i].assign(n_keys, default_value);
  }
  const auto& reducer_func = internal::MapreduceUtil::get_reducer_func<VD>(reducer);
  const auto& emit = [&](const size_t key, const VD& value) {
    const int thread_id = omp_get_thread_num();
    reducer_func(res_threads[thread_id][key], value);
  };
  source.for_each([&](const VS t) { mapper(t, emit); });

  // Node reduce.
  int step = 1;
  while (step < n_threads) {
    int i_end = n_threads - step;
    int i_step = step << 1;
#pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < i_end; i += i_step) {
      for (size_t j = 0; j < n_keys; j++) {
        reducer_func(res_threads[i][j], res_threads[i + step][j]);
      }
    }
    step <<= 1;
  }

  std::vector<VD> res(n_keys);
  std::vector<VD> res_local(n_keys);

  memcpy(res_local.data(), res_threads[0].data(), n_keys * sizeof(VD));

  // Cross-node tree reduce.
  MPI_Allreduce(
      res_local.data(),
      res.data(),
      n_keys,
      internal::MpiType<VD>::value,
      internal::MapreduceUtil::get_mpi_op(reducer),
      MPI_COMM_WORLD);

#pragma omp parallel for schedule(static)
  for (size_t i = 0; i < n_keys; i++) {
    reducer_func(dest[i], res[i]);
  }
}

}  // namespace blaze
