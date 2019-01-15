#pragma once

#include <vector>
#include "../mpi_type.h"
#include "../mpi_util.h"
#include "dist_hasher.h"

namespace fgpl {
namespace internal {
namespace hash {

template <class K, class V, class C, class H = std::hash<K>>
class DistHashBase {
 public:
  DistHashBase();

  void reserve(const size_t n_keys_min);

  size_t get_n_keys();

  size_t get_n_buckets();

  float get_load_factor();

  float get_max_load_factor() const { return max_load_factor; }

  void set_max_load_factor(const float max_load_factor);

  void clear();

  void clear_and_shrink();

 protected:
  int n_procs;

  int proc_id;

  H hasher;

  C local_data;

  std::vector<C> remote_data;

  std::vector<int> generate_shuffled_procs();

  int get_shuffled_id(const std::vector<int>& shuffled_procs);

 private:
  float max_load_factor;
};

template <class K, class V, class C, class H>
DistHashBase<K, V, C, H>::DistHashBase() {
  n_procs = internal::MpiUtil::get_n_procs();
  proc_id = internal::MpiUtil::get_proc_id();
  remote_data.resize(n_procs);
  max_load_factor = local_data.get_max_load_factor();
}

template <class K, class V, class C, class H>
void DistHashBase<K, V, C, H>::reserve(const size_t n_keys_min) {
  local_data.reserve(n_keys_min / n_procs);
  for (auto& remote_map : remote_data) {
    remote_map.reserve(n_keys_min / n_procs / n_procs);
  }
}

template <class K, class V, class C, class H>
size_t DistHashBase<K, V, C, H>::get_n_keys() {
  const size_t local_n_keys = local_data.get_n_keys();
  size_t n_keys;
  MPI_Allreduce(
      &local_n_keys, &n_keys, 1, internal::MpiType<size_t>::value, MPI_SUM, MPI_COMM_WORLD);
  return n_keys;
}

template <class K, class V, class C, class H>
size_t DistHashBase<K, V, C, H>::get_n_buckets() {
  const size_t local_n_buckets = local_data.get_n_buckets();
  size_t n_buckets;
  MPI_Allreduce(
      &local_n_buckets, &n_buckets, 1, internal::MpiType<size_t>::value, MPI_SUM, MPI_COMM_WORLD);
  return n_buckets;
}

template <class K, class V, class C, class H>
float DistHashBase<K, V, C, H>::get_load_factor() {
  return static_cast<float>(get_n_buckets()) / get_n_keys();
}

template <class K, class V, class C, class H>
void DistHashBase<K, V, C, H>::set_max_load_factor(const float max_load_factor) {
  this->max_load_factor = max_load_factor;
  local_data.set_max_load_factor(max_load_factor);
  for (auto& remote_map : remote_data) remote_map.set_max_load_factor(max_load_factor);
}

template <class K, class V, class C, class H>
std::vector<int> DistHashBase<K, V, C, H>::generate_shuffled_procs() {
  std::vector<int> res(n_procs);
  if (proc_id == 0) {
    // Fisher–Yates shuffle algorithm.
    for (int i = 0; i < n_procs; i++) res[i] = i;
    for (int i = res.size() - 1; i > 0; i--) {
      const int j = rand() % (i + 1);
      if (i != j) {
        const int tmp = res[i];
        res[i] = res[j];
        res[j] = tmp;
      }
    }
  }

  MPI_Bcast(res.data(), n_procs, MPI_INT, 0, MPI_COMM_WORLD);

  return res;
}

template <class K, class V, class C, class H>
int DistHashBase<K, V, C, H>::get_shuffled_id(const std::vector<int>& shuffled_procs) {
  for (int i = 0; i < n_procs; i++) {
    if (shuffled_procs[i] == proc_id) return i;
  }
  throw std::runtime_error("proc id does not exist in shuffled procs.");
}

template <class K, class V, class C, class H>
void DistHashBase<K, V, C, H>::clear() {
  local_data.clear();
  for (auto& remote_map : remote_data) remote_map.clear();
}

template <class K, class V, class C, class H>
void DistHashBase<K, V, C, H>::clear_and_shrink() {
  local_data.clear_and_shrink();
  for (auto& remote_map : remote_data) remote_map.clear_and_shrink();
}

}  // namespace hash
}  // namespace internal
}  // namespace fgpl
