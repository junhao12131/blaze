#pragma once

#include <functional>
#include "concurrent_hash_base.h"
#include "hash_set.h"

namespace fgpl {
namespace internal {
namespace hash {

template <class K, class H = std::hash<K>>
class ConcurrentHashSet : public ConcurrentHashBase<K, void, HashSet<K, H>, H> {
 public:
  void set(const K& key, const size_t hash_value);

  void async_set(const K& key, const size_t hash_value);

  void sync();

  void for_each_serial(
      const std::function<void(const K& key, const size_t hash_value)>& handler) const;

  using ConcurrentHashBase<K, void, HashSet<K, H>, H>::clear;

  using ConcurrentHashBase<K, void, HashSet<K, H>, H>::get_max_load_factor;

  using ConcurrentHashBase<K, void, HashSet<K, H>, H>::set_max_load_factor;

  template <class B>
  void serialize(B& buf) const;

  template <class B>
  void parse(B& buf);

 protected:
  using ConcurrentHashBase<K, void, HashSet<K, H>, H>::n_segments;

  using ConcurrentHashBase<K, void, HashSet<K, H>, H>::segments;

  using ConcurrentHashBase<K, void, HashSet<K, H>, H>::segment_locks;

  using ConcurrentHashBase<K, void, HashSet<K, H>, H>::thread_caches;
};

template <class K, class H>
void ConcurrentHashSet<K, H>::set(const K& key, const size_t hash_value) {
  const size_t segment_id = hash_value % n_segments;
  auto& lock = segment_locks[segment_id];
  HashSet<K, H>* segment_ptr = &segments[segment_id];
  omp_set_lock(&lock);
  segment_ptr->set(key, hash_value);
  omp_unset_lock(&lock);
}

template <class K, class H>
void ConcurrentHashSet<K, H>::async_set(const K& key, const size_t hash_value) {
  const size_t segment_id = hash_value % n_segments;
  auto& lock = segment_locks[segment_id];
  HashSet<K, H>* segment_ptr = &segments[segment_id];
  if (omp_test_lock(&lock)) {
    segment_ptr->set(key, hash_value);
    omp_unset_lock(&lock);
  } else {
    const int thread_id = omp_get_thread_num();
    thread_caches[thread_id].set(key, hash_value);
  }
}

template <class K, class H>
void ConcurrentHashSet<K, H>::sync() {
#pragma omp parallel
  {
    const int thread_id = omp_get_thread_num();
    const auto& handler = [&](const K& key, const size_t hash_value) {
      const size_t segment_id = hash_value % n_segments;
      auto& lock = segment_locks[segment_id];
      omp_set_lock(&lock);
      segments.at(segment_id).set(key, hash_value);
      omp_unset_lock(&lock);
    };
    thread_caches.at(thread_id).for_each(handler);
    thread_caches.at(thread_id).clear();
  }
}

template <class K, class H>
void ConcurrentHashSet<K, H>::for_each_serial(
    const std::function<void(const K& key, const size_t hash_value)>& handler) const {
  for (size_t segment_id = 0; segment_id < n_segments; segment_id++) {
    segments[segment_id].for_each(handler);
  }
}

template <class K, class H>
template <class B>
void ConcurrentHashSet<K, H>::serialize(B& buf) const {
  const float max_load_factor = get_max_load_factor();
  buf << n_segments << max_load_factor;
  for (size_t i = 0; i < n_segments; i++) {
    buf << segments[i];
  }
}

template <class K, class H>
template <class B>
void ConcurrentHashSet<K, H>::parse(B& buf) {
  clear();
  size_t n_segments_buf;
  float max_load_factor;
  buf >> n_segments_buf >> max_load_factor;
  set_max_load_factor(max_load_factor);
  HashSet<K, H> segment_buf;
  const auto& handler = [&](const K& key, const size_t hash_value) { set(key, hash_value); };
  for (size_t i = 0; i < n_segments_buf; i++) {
    buf >> segment_buf;
    segment_buf.for_each(handler);
  }
}

}  // namespace hash
}  // namespace internal
}  // namespace fgpl
