#ifndef BLAZE_DIST_RANGE_MAPREDUCER_H_
#define BLAZE_DIST_RANGE_MAPREDUCER_H_

#include <mpi.h>
#include <omp.h>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "dist_hash_map.h"
#include "dist_range.h"
#include "dist_vector.h"
#include "internal/mapreduce_util.h"
#include "internal/mpi_type.h"
#include "internal/mpi_util.h"
#include "internal/vector_mapreduce_wrapper.h"
#include "reducer.h"

namespace blaze {

template <class VS>
class DistRangeMapreducer {
 public:
  template <class VD>
  static void mapreduce(
      DistRange<VS>& source,
      const std::function<
          void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::string& reducer,
      std::vector<VD>& dest);

  template <class VD>
  static void mapreduce(
      DistRange<VS>& source,
      const std::function<
          void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      std::vector<VD>& dest);

  template <class VD>
  static void mapreduce(
      DistRange<VS>& source,
      const std::function<
          void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      DistVector<VD>& dest);

  template <class KD, class VD, class HD = std::hash<KD>>
  static void mapreduce(
      DistRange<VS>& source,
      const std::function<
          void(const VS value, const std::function<void(const KD&, const VD&)>& emit)>& mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      DistHashMap<KD, VD, HD>& dest);
};

template <class VS>
template <class VD>
void DistRangeMapreducer<VS>::mapreduce(
    DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    std::vector<VD>& dest) {
  internal::VectorMapreduceWrapper<VD> dest_wrapper(dest);
  const auto& emit = [&](const size_t key, const VD& value) {
    dest_wrapper.async_set(key, value, reducer);
  };
  const auto& handler = [&](const VS& value) { mapper(value, emit); };
  source.for_each(handler);
  dest_wrapper.sync(reducer);
}

template <class VS>
template <class VD>
void DistRangeMapreducer<VS>::mapreduce(
    DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::string& reducer,
    std::vector<VD>& dest) {
  internal::VectorMapreduceWrapper<VD> dest_wrapper(dest);
  const auto& reducer_func = internal::MapreduceUtil::get_reducer_func<VD>(reducer);
  const auto& emit = [&](const size_t key, const VD& value) {
    dest_wrapper.async_set(key, value, reducer_func);
  };
  const auto& handler = [&](const VS& value) { mapper(value, emit); };
  source.for_each(handler);
  dest_wrapper.sync(reducer);
}

template <class VS>
template <class VD>
void DistRangeMapreducer<VS>::mapreduce(
    DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    DistVector<VD>& dest) {
  const auto& emit = [&](const size_t key, const VD& value) {
    dest.async_set(key, value, reducer);
  };
  const auto& handler = [&](const VS& value) { mapper(value, emit); };
  source.for_each(handler);
  dest.sync(reducer);
}

template <class VS>
template <class KD, class VD, class HD>
void DistRangeMapreducer<VS>::mapreduce(
    DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const KD&, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    DistHashMap<KD, VD, HD>& dest) {
  const auto& emit = [&](const KD& key, const VD& value) { dest.async_set(key, value, reducer); };
  const auto& handler = [&](const VS& value) { mapper(value, emit); };
  source.for_each(handler);
  dest.sync(reducer);
}

}  // namespace blaze

#endif
