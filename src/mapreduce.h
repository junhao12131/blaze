#ifndef BLAZE_MAPREDUCE_H_
#define BLAZE_MAPREDUCE_H_

#include "dist_range_mapreducer.h"
#include "dist_vector_mapreducer.h"
#include "dist_hash_map_mapreducer.h"
#include "internal/mapreduce_util.h"

namespace blaze {

// From dist range source.
template <class VS, class VD>
void mapreduce(
    DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::string& reducer,
    std::vector<VD>& dest) {
  DistRangeMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

template <class VS, class VD>
void mapreduce(
    DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    std::vector<VD>& dest) {
  DistRangeMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

template <class VS, class VD>
void mapreduce(
    DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    blaze::DistVector<VD>& dest) {
  DistRangeMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

template <class VS, class KD, class VD, class HD = std::hash<KD>>
void mapreduce(
    DistRange<VS>& source,
    const std::function<
        void(const VS value, const std::function<void(const KD&, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    DistHashMap<KD, VD, HD>& dest) {
  DistRangeMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

// From dist vector source.
template <class VS, class VD>
void mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<void(
        const size_t key,
        const VS& value,
        const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::string& reducer,
    std::vector<VD>& dest) {
  DistVectorMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

template <class VS, class VD>
void mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<void(
        const size_t key,
        const VS& value,
        const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    std::vector<VD>& dest) {
  DistVectorMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

template <class VS, class VD>
void mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<void(
        const size_t key,
        const VS& value,
        const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    blaze::DistVector<VD>& dest) {
  DistVectorMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

template <class VS, class KD, class VD, class HD = std::hash<KD>>
void mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<void(
        const size_t key, const VS& value, const std::function<void(const KD&, const VD&)>& emit)>&
        mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    DistHashMap<KD, VD, HD>& dest) {
  DistVectorMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

// From dist hash map source.
template <class KS, class VS, class VD, class HS = std::hash<KS>>
void mapreduce(
    DistHashMap<KS, VS, HS>& source,
    const std::function<void(
        const KS& key, const VS& value, const std::function<void(const size_t, const VD&)>& emit)>&
        mapper,
    const std::string& reducer,
    std::vector<VD>& dest) {
  DistHashMapMapreducer<KS, VS, HS>::mapreduce(source, mapper, reducer, dest);
}

template <class KS, class VS, class VD, class HS = std::hash<KS>>
void mapreduce(
    DistHashMap<KS, VS, HS>& source,
    const std::function<void(
        const KS& key, const VS& value, const std::function<void(const size_t, const VD&)>& emit)>&
        mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    std::vector<VD>& dest) {
  DistHashMapMapreducer<KS, VS, HS>::mapreduce(source, mapper, reducer, dest);
}

template <class KS, class VS, class VD, class HS = std::hash<KS>>
void mapreduce(
    DistHashMap<KS, VS, HS>& source,
    const std::function<void(
        const KS& key, const VS& value, const std::function<void(const size_t, const VD&)>& emit)>&
        mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    blaze::DistVector<VD>& dest) {
  DistHashMapMapreducer<KS, VS, HS>::mapreduce(source, mapper, reducer, dest);
}

template <class KS,
          class VS,
          class KD,
          class VD,
          class HS = std::hash<KS>,
          class HD = std::hash<KD>>
void mapreduce(
    DistHashMap<KS, VS, HS>& source,
    const std::function<void(
        const KS& key, const VS& value, const std::function<void(const KD&, const VD&)>& emit)>&
        mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    DistHashMap<KD, VD, HD>& dest) {
  DistHashMapMapreducer<KS, VS, HS>::mapreduce(source, mapper, reducer, dest);
}

}  // namespace blaze

#endif
