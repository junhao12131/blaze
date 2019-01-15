#pragma once

#include <functional>
#include "../mpi_util.h"

namespace fgpl {
namespace internal {
namespace hash {
template <class K, class H>

class DistHasher {
 public:
  DistHasher() { n_procs_u = MpiUtil::get_n_procs(); }

  size_t operator()(const K& key) const { return hasher(key) / n_procs_u; }

 private:
  H hasher;

  size_t n_procs_u;
};
}  // namespace hash
}  // namespace internal
}  // namespace fgpl
