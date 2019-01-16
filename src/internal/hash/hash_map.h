#pragma once

#include <functional>
#include <vector>
#include "../../reducer.h"
#include "hash_base.h"

namespace blaze {
namespace internal {
namespace hash {

// A linear probing hash map that requires providing hash values when use.
template <class K, class V, class H = std::hash<K>>
class HashMap : public HashBase<K, V, H> {
 public:
  void set(
      const K& key,
      const size_t hash_value,
      const V& value,
      const std::function<void(V&, const V&)>& reducer);

  V get(const K& key, const size_t hash_value, const V& default_value) const;

  void for_each(const std::function<void(const K& key, const size_t hash_value, const V& value)>&
                    handler) const;

  using HashBase<K, V, H>::max_load_factor;

  using HashBase<K, V, H>::reserve_n_buckets;

  using HashBase<K, V, H>::clear;

  using HashBase<K, V, H>::reserve;

  template <class B>
  void serialize(B& buf) const;

  template <class B>
  void parse(B& buf);

 protected:
  using HashBase<K, V, H>::n_keys;

  using HashBase<K, V, H>::n_buckets;

  using HashBase<K, V, H>::buckets;

  using HashBase<K, V, H>::check_balance;
};

template <class K, class V, class H>
void HashMap<K, V, H>::set(
    const K& key,
    const size_t hash_value,
    const V& value,
    const std::function<void(V&, const V&)>& reducer) {
  size_t bucket_id = hash_value % n_buckets;
  size_t n_probes = 0;
  while (n_probes < n_buckets) {
    if (!buckets.at(bucket_id).filled) {
      buckets.at(bucket_id).fill(key, hash_value, value);
      n_keys++;
      if (n_buckets * max_load_factor <= n_keys) {
        reserve_n_buckets(static_cast<size_t>(n_buckets * 1.3));
      }
      break;
    } else if (buckets.at(bucket_id).key_equals(key, hash_value)) {
      reducer(buckets.at(bucket_id).value, value);
      break;
    } else {
      n_probes++;
      bucket_id = (bucket_id + 1) % n_buckets;
    }
  }
  check_balance(n_probes);
}

template <class K, class V, class H>
V HashMap<K, V, H>::get(const K& key, const size_t hash_value, const V& default_value) const {
  size_t bucket_id = hash_value % n_buckets;
  size_t n_probes = 0;
  while (n_probes < n_buckets) {
    if (!buckets.at(bucket_id).filled) {
      return default_value;
    } else if (buckets.at(bucket_id).key_equals(key, hash_value)) {
      return buckets.at(bucket_id).value;
    } else {
      n_probes++;
      bucket_id = (bucket_id + 1) % n_buckets;
    }
  }
  return default_value;
}

template <class K, class V, class H>
void HashMap<K, V, H>::for_each(
    const std::function<void(const K& key, const size_t hash_value, const V& value)>& handler)
    const {
  if (n_keys == 0) return;
  for (size_t i = 0; i < n_buckets; i++) {
    if (buckets.at(i).filled) {
      handler(buckets.at(i).key, buckets.at(i).hash_value, buckets.at(i).value);
    }
  }
}

template <class K, class V, class H>
template <class B>
void HashMap<K, V, H>::serialize(B& buf) const {
  buf << n_keys;
  const auto& handler = [&](const K& key, const size_t, const V& value) { buf << key << value; };
  for_each(handler);
}

template <class K, class V, class H>
template <class B>
void HashMap<K, V, H>::parse(B& buf) {
  size_t n_keys_buf;
  clear();
  buf >> n_keys_buf;
  reserve(n_keys_buf);
  H hasher;
  K key;
  V value;
  for (size_t i = 0; i < n_keys_buf; i++) {
    buf >> key >> value;
    set(key, hasher(key), value, Reducer<V>::keep);
  }
}

}  // namespace hash
}  // namespace internal
}  // namespace blaze
