#pragma once

#include "../../../vendor/hps/src/hps.h"
#include "../../gather.h"
#include "../../reducer.h"
#include "../mpi_util.h"
#include "concurrent_hash_map.h"
#include "dist_hash_base.h"

namespace fgpl {
namespace internal {
namespace hash {

template <class K, class V, class H = std::hash<K>>
class DistHashMap : public DistHashBase<K, V, ConcurrentHashMap<K, V, DistHasher<K, H>>, H> {
 public:
  void async_set(
      const K& key,
      const size_t hash_value,
      const V& value,
      const std::function<void(V&, const V&)>& reducer);

  void sync(const std::function<void(V&, const V&)>& reducer = Reducer<V>::overwrite);

  double get_local(const K& key, const size_t hash_value, const V& default_value) const;

  void for_each(
      const std::function<void(const K& key, const size_t hash_value, const V& value)>& handler)
      const;

  void for_each_serial(
      const std::function<void(const K& key, const size_t hash_value, const V& value)>& handler);

  template <class V2>
  V2 mapreduce(
      const std::function<V2(const K& key, const V& value)>& mapper,
      const std::function<void(V2&, const V2&)>& reducer,
      const V2& default_value);

 private:
  DistHasher<K, H> dist_hasher;

  using DistHashBase<K, V, ConcurrentHashMap<K, V, DistHasher<K, H>>, H>::hasher;

  using DistHashBase<K, V, ConcurrentHashMap<K, V, DistHasher<K, H>>, H>::n_procs;

  using DistHashBase<K, V, ConcurrentHashMap<K, V, DistHasher<K, H>>, H>::proc_id;

  using DistHashBase<K, V, ConcurrentHashMap<K, V, DistHasher<K, H>>, H>::local_data;

  using DistHashBase<K, V, ConcurrentHashMap<K, V, DistHasher<K, H>>, H>::remote_data;

  using DistHashBase<K, V, ConcurrentHashMap<K, V, DistHasher<K, H>>, H>::generate_shuffled_procs;

  using DistHashBase<K, V, ConcurrentHashMap<K, V, DistHasher<K, H>>, H>::get_shuffled_id;
};

template <class K, class V, class H>
void DistHashMap<K, V, H>::async_set(
    const K& key,
    const size_t hash_value,
    const V& value,
    const std::function<void(V&, const V&)>& reducer) {
  const size_t n_procs_u = n_procs;
  const size_t proc_id_u = proc_id;
  const size_t dest_proc_id = hash_value % n_procs_u;
  const size_t dist_hash_value = hash_value / n_procs_u;
  if (dest_proc_id == proc_id_u) {
    local_data.async_set(key, dist_hash_value, value, reducer);
  } else {
    remote_data[dest_proc_id].async_set(key, dist_hash_value, value, reducer);
  }
}

template <class K, class V, class H>
double DistHashMap<K, V, H>::get_local(
    const K& key, const size_t hash_value, const V& default_value) const {
  const size_t n_procs_u = n_procs;
  const size_t proc_id_u = proc_id;
  const size_t dest_proc_id = hash_value % n_procs_u;
  const size_t dist_hash_value = hash_value / n_procs_u;
  if (dest_proc_id == proc_id_u) {
    return local_data.get(key, dist_hash_value, default_value);
  } else {
    throw std::runtime_error("data not locally cached");
  }
}

template <class K, class V, class H>
void DistHashMap<K, V, H>::sync(const std::function<void(V&, const V&)>& reducer) {
  const auto& node_handler = [&](const K& key, const size_t hash_value, const V& value) {
    local_data.set(key, hash_value, value, reducer);
  };

  // Accelerate overall network transfer through randomization.
  const auto& shuffled_procs = generate_shuffled_procs();
  const int shuffled_id = get_shuffled_id(shuffled_procs);

  std::vector<std::string> send_bufs(n_procs);
  std::vector<std::string> recv_bufs(n_procs);
  const size_t BUF_SIZE = 1 << 20;
  char send_buf_char[BUF_SIZE];
  char recv_buf_char[BUF_SIZE];
  MPI_Request reqs[2];
  MPI_Status stats[2];

#pragma omp parallel for schedule(dynamic, 1)
  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    hps::to_string(remote_data[dest_proc_id], send_bufs[i]);
  }

  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    const int src_proc_id = shuffled_procs[(shuffled_id + n_procs - i) % n_procs];
    remote_data[dest_proc_id].sync(reducer);
    const auto& send_buf = send_bufs[i];
    auto& recv_buf = recv_bufs[i];
    remote_data[dest_proc_id].clear();
    size_t send_cnt = send_buf.size();
    size_t recv_cnt = 0;
    MPI_Irecv(&recv_cnt, 1, MpiType<size_t>::value, src_proc_id, 0, MPI_COMM_WORLD, &reqs[0]);
    MPI_Isend(&send_cnt, 1, MpiType<size_t>::value, dest_proc_id, 0, MPI_COMM_WORLD, &reqs[1]);
    MPI_Waitall(2, reqs, stats);
    size_t send_pos = 0;
    size_t recv_pos = 0;
    recv_buf.clear();
    recv_buf.reserve(recv_cnt);
    while (send_pos < send_cnt || recv_pos < recv_cnt) {
      const int recv_trunk_cnt = (recv_cnt - recv_pos >= BUF_SIZE) ? BUF_SIZE : recv_cnt - recv_pos;
      const int send_trunk_cnt = (send_cnt - send_pos >= BUF_SIZE) ? BUF_SIZE : send_cnt - send_pos;
      if (recv_trunk_cnt > 0) {
        MPI_Irecv(
            recv_buf_char, recv_trunk_cnt, MPI_CHAR, src_proc_id, 1, MPI_COMM_WORLD, &reqs[0]);
        recv_pos += recv_trunk_cnt;
      }
      if (send_trunk_cnt > 0) {
        send_buf.copy(send_buf_char, send_trunk_cnt, send_pos);
        MPI_Issend(
            send_buf_char, send_trunk_cnt, MPI_CHAR, dest_proc_id, 1, MPI_COMM_WORLD, &reqs[1]);
        send_pos += send_trunk_cnt;
      }
      MPI_Waitall(2, reqs, stats);
      recv_buf.append(recv_buf_char, recv_trunk_cnt);
    }
  }

  size_t n_keys = local_data.get_n_keys();
#pragma omp parallel for schedule(dynamic, 1)
  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    hps::from_string(recv_bufs[i], remote_data[dest_proc_id]);
#pragma omp atomic
    n_keys += remote_data[dest_proc_id].get_n_keys();
  }

  local_data.reserve(n_keys);

#pragma omp parallel for schedule(dynamic, 1)
  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    remote_data[dest_proc_id].for_each_serial(node_handler);
    remote_data[dest_proc_id].clear();
  }

  local_data.sync(reducer);
}

template <class K, class V, class H>
void DistHashMap<K, V, H>::for_each(
    const std::function<void(const K& key, const size_t hash_value, const V& value)>& handler)
    const {
  local_data.for_each(handler);
}

template <class K, class V, class H>
void DistHashMap<K, V, H>::for_each_serial(
    const std::function<void(const K& key, const size_t hash_value, const V& value)>& handler) {
  const auto& local_maps = gather(local_data);
  for (int i = 0; i < n_procs; i++) {
    local_maps[i].for_each_serial(handler);
  }
}

template <class K, class V, class H>
template <class V2>
V2 DistHashMap<K, V, H>::mapreduce(
    const std::function<V2(const K& key, const V& value)>& mapper,
    const std::function<void(V2&, const V2&)>& reducer,
    const V2& default_value) {
  const int n_threads = omp_get_max_threads();
  std::vector<V2> res_thread(n_threads, default_value);
  for_each([&](const K& key, const size_t, const V& value) {
    const int thread_id = omp_get_thread_num();
    reducer(res_thread[thread_id], mapper(key, value));
  });
  V2 res_local;
  V2 res;
  res_local = res_thread[0];
  for (int i = 1; i < n_threads; i++) res_local += res_thread[i];
  std::vector<V2> res_locals = gather(res_local);
  res = res_locals[0];
  const int n_procs = MpiUtil::get_n_procs();
  for (int i = 1; i < n_procs; i++) reducer(res, res_locals[i]);
  return res;
}

}  // namespace hash
}  // namespace internal
}  // namespace fgpl
