#ifndef BLAZE_DISTRIBUTE_H_
#define BLAZE_DISTRIBUTE_H_

#include <functional>
#include <vector>

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
}

#endif
