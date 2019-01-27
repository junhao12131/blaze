#ifndef BLAZE_COLLECT_H_
#define BLAZE_COLLECT_H_

#include <functional>
#include <vector>

#include "dist_hash_map.h"
#include "dist_vector.h"
#include "gather.h"

namespace blaze {
template <class V>
std::vector<V> collect(blaze::DistVector<V>& input) {
  const size_t n_keys = input.size();
  std::vector<V> output(n_keys);

  const size_t n_procs_u = internal::MpiUtil::get_n_procs();
  std::vector<V> local(n_keys / n_procs_u + 1);
  input.for_each([&](const size_t key, const V& value) { local[key / n_procs_u] = value; });
  std::vector<std::vector<V>> gathered = blaze::gather(local);

#pragma omp parallel for
  for (size_t i = 0; i < n_keys; i++) {
    output[i] = gathered[i % n_procs_u][i / n_procs_u];
  }

  return output;
}

template <class K, class V, class H = std::hash<K>>
std::unordered_map<K, V, H> collect(DistHashMap<K, V, H>& input) {
  std::unordered_map<K, V, H> output;
  output.reserve(input.get_n_keys());
  input.for_each_serial([&](const K& key, const size_t, const V& value) { output[key] = value; });
  return output;
}
}

#endif
