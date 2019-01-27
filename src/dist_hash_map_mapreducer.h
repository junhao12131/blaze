#ifndef BLAZE_DIST_HASH_MAP_MAPREDUCER_H_
#define BLAZE_DIST_HASH_MAP_MAPREDUCER_H_

namespace blaze {
template <class KS, class VS, class HS = std::hash<KS>>
class DistHashMapMapreducer {
 public:
  template <class VD>
  void mapreduce(
      DistHashMap<KS, VS, HS>& source,
      const std::function<void(
          const KS& key,
          const VS& value,
          const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      std::vector<VD>& dest);

  template <class VD>
  void mapreduce(
      DistHashMap<KS, VS, HS>& source,
      const std::function<void(
          const KS& key,
          const VS& value,
          const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::string& reducer,
      std::vector<VD>& dest);

  template <class VD>
  void mapreduce(
      DistHashMap<KS, VS, HS>& source,
      const std::function<void(
          const KS& key,
          const VS& value,
          const std::function<void(const size_t, const VD&)>& emit)>& mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      DistVector<VD>& dest);

  template <class KD, class VD, class HD = std::hash<KD>>
  void mapreduce(
      DistHashMap<KS, VS, HS>& source,
      const std::function<void(
          const KS& key, const VS& value, const std::function<void(const KD&, const VD&)>& emit)>&
          mapper,
      const std::function<void(VD&, const VD&)>& reducer,
      DistHashMap<KD, VD, HD>& dest);
};

template <class KS, class VS, class HS>
template <class VD>
void DistHashMapMapreducer<KS, VS, HS>::mapreduce(
    DistHashMap<KS, VS, HS>& source,
    const std::function<void(
        const KS& key, const VS& value, const std::function<void(const size_t, const VD&)>& emit)>&
        mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    std::vector<VD>& dest) {
  internal::VectorMapreduceWrapper<VD> dest_wrapper(dest);
  const auto& emit = [&](const size_t key, const VD& value) {
    dest_wrapper.async_set(key, value, reducer);
  };
  const auto& handler = [&](const KS& key, const VS& value) { mapper(key, value, emit); };
  source.for_each(handler);
  dest_wrapper.sync(reducer);
}

template <class KS, class VS, class HS>
template <class VD>
void DistHashMapMapreducer<KS, VS, HS>::mapreduce(
    DistHashMap<KS, VS, HS>& source,
    const std::function<void(
        const KS& key, const VS& value, const std::function<void(const size_t, const VD&)>& emit)>&
        mapper,
    const std::string& reducer,
    std::vector<VD>& dest) {
  internal::VectorMapreduceWrapper<VD> dest_wrapper(dest);
  const auto& reducer_func = internal::MapreduceUtil::get_reducer_func<VD>(reducer);
  const auto& emit = [&](const size_t key, const VD& value) {
    dest_wrapper.async_set(key, value, reducer_func);
  };
  const auto& handler = [&](const KS& key, const VS& value) { mapper(key, value, emit); };
  source.for_each(handler);
  dest_wrapper.sync(reducer);
}

template <class KS, class VS, class HS>
template <class VD>
void DistHashMapMapreducer<KS, VS, HS>::mapreduce(
    DistHashMap<KS, VS, HS>& source,
    const std::function<void(
        const KS& key, const VS& value, const std::function<void(const size_t, const VD&)>& emit)>&
        mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    blaze::DistVector<VD>& dest) {
  const auto& emit = [&](const size_t key, const VD& value) {
    dest.async_set(key, value, reducer);
  };
  const auto& handler = [&](const KS& key, const VS& value) { mapper(key, value, emit); };
  source.for_each(handler);
  dest.sync(reducer);
}

template <class KS, class VS, class HS>
template <class KD, class VD, class HD>
void DistHashMapMapreducer<KS, VS, HS>::mapreduce(
    DistHashMap<KS, VS, HS>& source,
    const std::function<void(
        const KS& key, const VS& value, const std::function<void(const KD&, const VD&)>& emit)>&
        mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    DistHashMap<KD, VD, HD>& dest) {
  const auto& emit = [&](const KD& key, const VD& value) { dest.async_set(key, value, reducer); };
  const auto& handler = [&](const KS& key, const VS& value) { mapper(key, value, emit); };
  source.for_each(handler);
  dest.sync(reducer);
}
}

#endif
