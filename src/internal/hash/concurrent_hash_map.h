#ifndef BLAZE_INTERNAL_HASH_CONCURRENT_HASH_MAP_H_
#define BLAZE_INTERNAL_HASH_CONCURRENT_HASH_MAP_H_

#include <functional>
#include "concurrent_hash_base.h"
#include "hash_map.h"

namespace blaze {
namespace internal {
namespace hash {

template <class K, class V, class H = std::hash<K>>
class ConcurrentHashMap : public ConcurrentHashBase<K, V, HashMap<K, V, H>, H> {
 public:
  void set(
      const K& key,
      const size_t hash_value,
      const V& value,
      const std::function<void(V&, const V&)>& reducer);

  void async_set(
      const K& key,
      const size_t hash_value,
      const V& value,
      const std::function<void(V&, const V&)>& reducer);

  V get(const K& key, const size_t hash_value, const V& default_value) const;

  void sync(const std::function<void(V&, const V&)>& reducer = Reducer<V>::overwrite);

  void for_each(const std::function<void(const K& key, const size_t hash_value, const V& value)>&
                    handler) const;

  void for_each_serial(
      const std::function<void(const K& key, const size_t hash_value, const V& value)>& handler)
      const;

  using ConcurrentHashBase<K, V, HashMap<K, V, H>, H>::clear;

  using ConcurrentHashBase<K, V, HashMap<K, V, H>, H>::get_max_load_factor;

  using ConcurrentHashBase<K, V, HashMap<K, V, H>, H>::set_max_load_factor;

  template <class B>
  void serialize(B& buf) const;

  template <class B>
  void parse(B& buf);

 protected:
  using ConcurrentHashBase<K, V, HashMap<K, V, H>, H>::n_segments;

  using ConcurrentHashBase<K, V, HashMap<K, V, H>, H>::segments;

  using ConcurrentHashBase<K, V, HashMap<K, V, H>, H>::segment_locks;

  using ConcurrentHashBase<K, V, HashMap<K, V, H>, H>::thread_caches;
};

template <class K, class V, class H>
void ConcurrentHashMap<K, V, H>::set(
    const K& key,
    const size_t hash_value,
    const V& value,
    const std::function<void(V&, const V&)>& reducer) {
  const size_t segment_id = hash_value % n_segments;
  auto& lock = segment_locks[segment_id];
  HashMap<K, V, H>* segment_ptr = &segments[segment_id];
  omp_set_lock(&lock);
  segment_ptr->set(key, hash_value, value, reducer);
  omp_unset_lock(&lock);
}

template <class K, class V, class H>
void ConcurrentHashMap<K, V, H>::async_set(
    const K& key,
    const size_t hash_value,
    const V& value,
    const std::function<void(V&, const V&)>& reducer) {
  const size_t segment_id = hash_value % n_segments;
  auto& lock = segment_locks[segment_id];
  HashMap<K, V, H>* segment_ptr = &segments[segment_id];
  if (omp_test_lock(&lock)) {
    segment_ptr->set(key, hash_value, value, reducer);
    omp_unset_lock(&lock);
  } else {
    const int thread_id = omp_get_thread_num();
    thread_caches[thread_id].set(key, hash_value, value, reducer);
  }
}

template <class K, class V, class H>
V ConcurrentHashMap<K, V, H>::get(
    const K& key, const size_t hash_value, const V& default_value) const {
  const size_t segment_id = hash_value % n_segments;
  V res = segments[segment_id].get(key, hash_value, default_value);
  return res;
}

template <class K, class V, class H>
void ConcurrentHashMap<K, V, H>::sync(const std::function<void(V&, const V&)>& reducer) {
#pragma omp parallel
  {
    const int thread_id = omp_get_thread_num();
    const auto& handler = [&](const K& key, const size_t hash_value, const V& value) {
      const size_t segment_id = hash_value % n_segments;
      auto& lock = segment_locks[segment_id];
      omp_set_lock(&lock);
      segments.at(segment_id).set(key, hash_value, value, reducer);
      omp_unset_lock(&lock);
    };
    thread_caches.at(thread_id).for_each(handler);
    thread_caches.at(thread_id).clear();
  }
}

template <class K, class V, class H>
void ConcurrentHashMap<K, V, H>::for_each(
    const std::function<void(const K& key, const size_t hash_value, const V& value)>& handler)
    const {
#pragma omp parallel for schedule(dynamic, 1)
  for (size_t segment_id = 0; segment_id < n_segments; segment_id++) {
    segments[segment_id].for_each(handler);
  }
}

template <class K, class V, class H>
void ConcurrentHashMap<K, V, H>::for_each_serial(
    const std::function<void(const K& key, const size_t hash_value, const V& value)>& handler)
    const {
  for (size_t segment_id = 0; segment_id < n_segments; segment_id++) {
    segments[segment_id].for_each(handler);
  }
}

template <class K, class V, class H>
template <class B>
void ConcurrentHashMap<K, V, H>::serialize(B& buf) const {
  const float max_load_factor = get_max_load_factor();
  buf << n_segments << max_load_factor;
  for (size_t i = 0; i < n_segments; i++) {
    buf << segments[i];
  }
}

template <class K, class V, class H>
template <class B>
void ConcurrentHashMap<K, V, H>::parse(B& buf) {
  clear();
  size_t n_segments_buf;
  float max_load_factor;
  buf >> n_segments_buf >> max_load_factor;
  set_max_load_factor(max_load_factor);
  HashMap<K, V, H> segment_buf;
  const auto& handler = [&](const K& key, const size_t hash_value, const V& value) {
    set(key, hash_value, value, Reducer<V>::keep);
  };
  for (size_t i = 0; i < n_segments_buf; i++) {
    buf >> segment_buf;
    segment_buf.for_each(handler);
  }
}

}  // namespace hash
}  // namespace internal
}  // namespace blaze

#endif
