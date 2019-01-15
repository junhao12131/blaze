#pragma once

#include "../../../vendor/hps/src/hps.h"
#include "../../gather.h"
#include "../mpi_util.h"
#include "concurrent_hash_set.h"
#include "dist_hash_base.h"

namespace fgpl {
namespace internal {
namespace hash {

template <class K, class H = std::hash<K>>
class DistHashSet : public DistHashBase<K, void, ConcurrentHashSet<K, DistHasher<K, H>>, H> {
 public:
  void async_set(const K& key, const size_t hash_value);

  void sync();

  void for_each_serial(const std::function<void(const K& key, const size_t hash_value)>& handler);

 private:
  DistHasher<K, H> dist_hasher;

  using DistHashBase<K, void, ConcurrentHashSet<K, DistHasher<K, H>>, H>::hasher;

  using DistHashBase<K, void, ConcurrentHashSet<K, DistHasher<K, H>>, H>::n_procs;

  using DistHashBase<K, void, ConcurrentHashSet<K, DistHasher<K, H>>, H>::proc_id;

  using DistHashBase<K, void, ConcurrentHashSet<K, DistHasher<K, H>>, H>::local_data;

  using DistHashBase<K, void, ConcurrentHashSet<K, DistHasher<K, H>>, H>::remote_data;

  using DistHashBase<K, void, ConcurrentHashSet<K, DistHasher<K, H>>, H>::generate_shuffled_procs;

  using DistHashBase<K, void, ConcurrentHashSet<K, DistHasher<K, H>>, H>::get_shuffled_id;
};

template <class K, class H>
void DistHashSet<K, H>::async_set(const K& key, const size_t hash_value) {
  const size_t n_procs_u = n_procs;
  const size_t proc_id_u = proc_id;
  const size_t dest_proc_id = hash_value % n_procs_u;
  const size_t dist_hash_value = hash_value / n_procs_u;
  if (dest_proc_id == proc_id_u) {
    local_data.async_set(key, dist_hash_value);
  } else {
    remote_data[dest_proc_id].async_set(key, dist_hash_value);
  }
}

template <class K, class H>
void DistHashSet<K, H>::sync() {
  const auto& node_handler = [&](const K& key, const size_t hash_value) {
    local_data.async_set(key, hash_value);
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
    remote_data[dest_proc_id].sync();
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


  local_data.sync();
}

template <class K, class H>
void DistHashSet<K, H>::for_each_serial(
    const std::function<void(const K& key, const size_t hash_value)>& handler) {
  const auto& local_sets = gather(local_data);
  for (int i = 0; i < n_procs; i++) {
    local_sets[i].for_each_serial(handler);
  }
}

}  // namespace hash
}  // namespace internal
}  // namespace fgpl
