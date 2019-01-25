#ifndef BLAZE_COLLECT_H_
#define BLAZE_COLLECT_H_

#include <functional>
#include <vector>

#include "dist_vector.h"
#include "gather.h"

namespace blaze {
template <class V>
std::vector<V> collect(blaze::DistVector<V>& input) {
  const size_t n_keys = input.size();
  std::vector<V> output(n_keys);

  const size_t n_proc_u = internal::MpiUtil::get_n_procs();
  std::vector<V> local(n_keys / n_proc_u + 1);
  input.for_each([&](const size_t key, const V& value) { local[key / n_proc_u] = value; });
  std::vector<std::vector<V>> gathered = blaze::gather(local);

#pragma omp parallel for
  for (size_t i = 0; i < n_keys; i++) {
    output[i] = gathered[i % n_proc_u][i / n_proc_u];
  }

  return output;
}
}

#endif
