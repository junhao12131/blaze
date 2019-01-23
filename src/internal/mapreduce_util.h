#ifndef BLAZE_INTERNAL_MAPREDUCE_UTIL_H_
#define BLAZE_INTERNAL_MAPREDUCE_UTIL_H_

#include <mpi.h>

#include "../reducer.h"

namespace blaze {
namespace internal {
class MapreduceUtil {
 public:
  template <class V>
  static std::function<void(V&, const V&)> get_reducer_func(const std::string& reducer) {
    if (reducer == "sum") {
      return Reducer<V>::sum;
    } else if (reducer == "prod") {
      return Reducer<V>::prod;
    } else if (reducer == "max") {
      return Reducer<V>::max;
    } else if (reducer == "min") {
      return Reducer<V>::min;
    } else if (reducer == "overwrite") {
      return Reducer<V>::overwrite;
    } else if (reducer == "keep") {
      return Reducer<V>::keep;
    }

    throw std::invalid_argument("invalid reducer: ");
  }

  static MPI_Op get_mpi_op(const std::string& reducer) {
    if (reducer == "sum") {
      return MPI_SUM;
    } else if (reducer == "prod") {
      return MPI_PROD;
    } else if (reducer == "max") {
      return MPI_MAX;
    } else if (reducer == "min") {
      return MPI_MIN;
    }

    throw std::invalid_argument("invalid reducer");
  }
};

}  // namespace internal
}  // namespace blaze

#endif
