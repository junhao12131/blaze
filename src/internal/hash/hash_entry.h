#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>

namespace blaze {
namespace internal {
namespace hash {

template <class K, class V>
class HashEntry {
 public:
  K key;

  size_t hash_value;

  V value;

  bool filled;

  HashEntry() : filled(false){};

  void fill(const K& key, const size_t hash_value, const V& value) {
    this->key = key;
    this->hash_value = hash_value;
    this->value = value;
    filled = true;
  }

  bool key_equals(const K& key, const size_t hash_value) const {
    return this->hash_value == hash_value && this->key == key;
  }
};

// For hash set.
template <class K>
class HashEntry<K, void> {
 public:
  K key;

  size_t hash_value;

  bool filled;

  HashEntry() : filled(false){};

  void fill(const K& key, const size_t hash_value) {
    this->key = key;
    this->hash_value = hash_value;
    filled = true;
  }

  bool key_equals(const K& key, const size_t hash_value) const {
    return this->hash_value == hash_value && this->key == key;
  }
};
}  // namespace hash
}  // namespace internal
}  // namespace blaze
