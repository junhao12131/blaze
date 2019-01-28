#ifndef BLAZE_DIST_VECTOR_MAPREDUCER_H_
#define BLAZE_DIST_VECTOR_MAPREDUCER_H_

#include <functional>
#include <limits>
#include <string>
#include <vector>
#include <vector>

#include "dist_hash_map.h"
#include "dist_vector.h"
#include "internal/mapreduce_util.h"
#include "internal/mpi_type.h"
#include "internal/mpi_util.h"
#include "internal/vector_mapreduce_wrapper.h"

namespace blaze {

template <class VS>
class DistVectorMapreducer {
 public:
  template <class VD>
  static void mapreduce(
      blaze::DistVector<VS>& source,
      const std::function<void(
          const size_t key,
          const VS& value,
          const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      std::vector<VD>& dest);

  template <class VD>
  static void mapreduce(
      blaze::DistVector<VS>& source,
      const std::function<void(
          const size_t key,
          const VS& value,
          const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::string& reducer,
      std::vector<VD>& dest);

  template <class VD>
  static void mapreduce(
      blaze::DistVector<VS>& source,
      const std::function<void(
          const size_t key,
          const VS& value,
          const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      blaze::DistVector<VD>& dest);

  template <class KD, class VD, class HD = std::hash<KD>>
  static void mapreduce(
      blaze::DistVector<VS>& source,
      const std::function<void(
          const size_t key,
          const VS& value,
          const std::function<void(const KD&, const VD&)>& emit)>& mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      blaze::DistHashMap<KD, VD, HD>& dest);
};

template <class VS>
template <class VD>
void DistVectorMapreducer<VS>::mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<void(
        const size_t key,
        const VS& value,
        const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    std::vector<VD>& dest) {
  internal::VectorMapreduceWrapper<VD> dest_wrapper(dest);
  const auto& emit = [&](const size_t key, const VD& value) {
    dest_wrapper.async_set(key, value, reducer);
  };
  const auto& handler = [&](const size_t key, const VS& value) { mapper(key, value, emit); };
  source.for_each(handler);
  dest_wrapper.sync(reducer);
}

template <class VS>
template <class VD>
void DistVectorMapreducer<VS>::mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<void(
        const size_t key,
        const VS& value,
        const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::string& reducer,
    std::vector<VD>& dest) {
  internal::VectorMapreduceWrapper<VD> dest_wrapper(dest);
  const auto& reducer_func = internal::MapreduceUtil::get_reducer_func<VD>(reducer);
  const auto& emit = [&](const size_t key, const VD& value) {
    dest_wrapper.async_set(key, value, reducer_func);
  };
  const auto& handler = [&](const size_t key, const VS& value) { mapper(key, value, emit); };
  source.for_each(handler);
  dest_wrapper.sync(reducer);
}

template <class VS>
template <class VD>
void DistVectorMapreducer<VS>::mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<void(
        const size_t key,
        const VS& value,
        const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    blaze::DistVector<VD>& dest) {
  const auto& emit = [&](const size_t key, const VD& value) {
    dest.async_set(key, value, reducer);
  };
  const auto& handler = [&](const size_t key, const VS& value) { mapper(key, value, emit); };
  source.for_each(handler);
  dest.sync(reducer);
}

template <class VS>
template <class KD, class VD, class HD>
void DistVectorMapreducer<VS>::mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<void(
        const size_t key, const VS& value, const std::function<void(const KD&, const VD&)>& emit)>&
        mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    blaze::DistHashMap<KD, VD, HD>& dest) {
  const auto& emit = [&](const KD& key, const VD& value) { dest.async_set(key, value, reducer); };
  const auto& handler = [&](const size_t key, const VS& value) { mapper(key, value, emit); };
  source.for_each(handler);
  dest.sync(reducer);
}

}  // namespace blaze

#endif
