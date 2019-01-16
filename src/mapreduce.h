#pragma once

#include "dist_range_mapreducer.h"
#include "dist_vector_mapreducer.h"

namespace blaze {

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
    blaze::DistVector<VS>& source,
    const std::function<
        void(const size_t key, const VS& value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::string& reducer,
    std::vector<VD>& dest) {
  DistVectorMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

template <class VS, class VD>
void mapreduce(
    blaze::DistVector<VS>& source,
    const std::function<
        void(const size_t key, const VS& value, const std::function<void(const size_t, const VD&)>& emit)>& mapper,
    const std::function<void(VD&, const VD&)>& reducer,
    blaze::DistVector<VD>& dest) {
  DistVectorMapreducer<VS>::mapreduce(source, mapper, reducer, dest);
}

/*
template <class K, class V, class H, class T>
void mapreduce(const blaze::DistMap<K, V, H>& source, mapper, reducer, std::vector<T>& dest);

template <class VS, T>
void mapreduce(const blaze::DistRange<VS>& source, mapper, reducer, blaze::DistVector<T>& dest);

template <class K, class V, class H, class T>
void mapreduce(const blaze::DistMap<K, V, H>& source, mapper, reducer, blaze::DistVector<T>& dest);

template <class T, class K, class V, class H>
void mapreduce(const blaze::DistRange<T>& source, mapper, reducer, blaze::DistMap<K, V, H>& dest);

template <class T, class K, class V, class H>
void mapreduce(const blaze::DistVector<T>& source, mapper, reducer, blaze::DistMap<K, V, H>& dest);

template <class KS, class VS, class HS, class K, class V, class H>
void mapreduce(
    const blaze::DistMap<KS, VS, HS>& source, mapper, reducer, blaze::DistMap<K, V, H>& dest);
    */

}  // namespace blaze
