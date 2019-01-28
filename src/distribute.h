#ifndef BLAZE_DISTRIBUTE_H_
#define BLAZE_DISTRIBUTE_H_

#include <functional>
#include <vector>

#include "dist_hash_map.h"
#include "dist_range.h"
#include "dist_vector.h"

namespace blaze {
template <class V>
blaze::DistVector<V> distribute(const std::vector<V>& input) {
  blaze::DistRange<size_t> indices(0, input.size());
  blaze::DistVector<V> output(input.size());
  indices.for_each([&](const size_t index) { output.async_set(index, input[index]); });
  output.sync();
  return output;
}

template <class K, class V, class H = std::hash<K>>
blaze::DistHashMap<K, V, H> distribute(const std::unordered_map<K, V, H>& input) {
  blaze::DistRange<size_t> indices;
  blaze::DistHashMap<K, V, H> output;
  output.reserve(input.size());
  H hasher;
  for (const auto& kv : input) {
    size_t hash_value = hasher(kv.first);
    if (indices.is_local(hash_value)) {
      output.async_set(kv.first, kv.second);
    }
  }
  output.sync();
  return output;
}
}

#endif
