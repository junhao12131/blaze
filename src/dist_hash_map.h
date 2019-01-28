#ifndef BLAZE_DIST_HASH_MAP_H_
#define BLAZE_DIST_HASH_MAP_H_

#include "internal/hash/dist_hash_map.h"
#include "reducer.h"

namespace blaze {
template <class K, class V, class H = std::hash<K>>
class DistHashMap : public internal::hash::DistHashMap<K, V, H> {
 public:
  void async_set(
      const K& key,
      const V& value,
      const std::function<void(V&, const V&)>& reducer = Reducer<V>::overwrite) {
    internal::hash::DistHashMap<K, V, H>::async_set(key, hasher(key), value, reducer);
  }

  double get_local(const K& key, const V& default_value) const {
    return internal::hash::DistHashMap<K, V, H>::get_local(key, hasher(key), default_value);
  }

 private:
  H hasher;

  using internal::hash::DistHashMap<K, V, H>::async_set;

  using internal::hash::DistHashMap<K, V, H>::get_local;
};
}  // namespace blaze

#endif
