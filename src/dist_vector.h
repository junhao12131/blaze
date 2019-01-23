#pragma once

#include <functional>
#include <vector>

#include "../vendor/hps/src/hps.h"
#include "broadcast.h"
#include "internal/concurrent_vector.h"
#include "internal/hash/concurrent_hash_map.h"
#include "internal/mpi_type.h"
#include "internal/mpi_util.h"
#include "reducer.h"

namespace blaze {

template <class V>
class DistVector {
 public:
  DistVector();

  DistVector(const size_t n, const V& value = V());

  void resize(const size_t n, const V& value = V());

  static bool is_local_id(const size_t id);

  size_t size() const { return n; }

  void async_set(
      const size_t key,
      const V& value,
      const std::function<void(V&, const V&)>& reducer = Reducer<V>::overwrite);

  void sync(const std::function<void(V&, const V&)>& reducer = Reducer<V>::overwrite);

  DistVector<V>& operator-=(const DistVector<V>& rhs) {
    local_data -= rhs.local_data;
    return *this;
  }

  void for_each(const std::function<void(const size_t key, V& value)>& handler);

  std::vector<V> top_k(const size_t k, const std::function<bool(const V&, const V&)>& compare);

 private:
  size_t n;

  int n_procs;

  int proc_id;

  std::hash<size_t> hasher;

  internal::ConcurrentVector<V> local_data;

  std::vector<internal::hash::ConcurrentHashMap<size_t, V, std::hash<size_t>>> remote_data;

  void init();
};

template <class V>
DistVector<V>::DistVector() {
  init();
}

template <class V>
DistVector<V>::DistVector(const size_t n, const V& value) {
  init();
  resize(n, value);
}

template <class V>
bool DistVector<V>::is_local_id(const size_t id) {
  static size_t n_procs_u = internal::MpiUtil::get_n_procs();
  static size_t proc_id_u = internal::MpiUtil::get_proc_id();
  return id % n_procs_u == proc_id_u;
}

template <class V>
void DistVector<V>::init() {
  n_procs = internal::MpiUtil::get_n_procs();
  proc_id = internal::MpiUtil::get_proc_id();
  remote_data.resize(n_procs);
}

template <class V>
void DistVector<V>::resize(const size_t n, const V& value) {
  this->n = n;

  static size_t n_procs_u = internal::MpiUtil::get_n_procs();
  static size_t proc_id_u = internal::MpiUtil::get_proc_id();
  size_t n_local = n / n_procs_u;
  if (n % n_procs_u > proc_id_u) n_local++;

  local_data.resize(n_local, value);
  for (auto& remote_map : remote_data) {
    remote_map.reserve(n / n_procs / n_procs + 1);
  }
}

template <class V>
void DistVector<V>::async_set(
    const size_t key, const V& value, const std::function<void(V&, const V&)>& reducer) {
  const size_t n_procs_u = n_procs;
  const size_t proc_id_u = proc_id;
  const size_t dest_proc_id = key % n_procs_u;
  const size_t dest_key = key / n_procs_u;
  if (dest_proc_id == proc_id_u) {
    local_data.async_set(dest_key, value, reducer);
  } else {
    remote_data[dest_proc_id].async_set(dest_key, hasher(dest_key), value, reducer);
  }
}

template <class V>
void DistVector<V>::sync(const std::function<void(V&, const V&)>& reducer) {
  const auto& node_handler = [&](const size_t key, const size_t, const V& value) {
    local_data.async_set(key, value, reducer);
  };

  // Accelerate overall network transfer through randomization.
  const auto& shuffled_procs = internal::MpiUtil::generate_shuffled_procs();
  const int shuffled_id = internal::MpiUtil::get_shuffled_id(shuffled_procs);

  std::vector<std::string> send_bufs(n_procs);
  std::vector<std::string> recv_bufs(n_procs);
  const size_t BUF_SIZE = 1 << 20;
  char send_buf_char[BUF_SIZE];
  char recv_buf_char[BUF_SIZE];
  MPI_Request reqs[2];
  MPI_Status stats[2];

  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    remote_data[dest_proc_id].sync(reducer);
  }

#pragma omp parallel for schedule(dynamic, 1)
  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    hps::to_string(remote_data[dest_proc_id], send_bufs[i]);
    remote_data[dest_proc_id].clear();
  }

  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    const int src_proc_id = shuffled_procs[(shuffled_id + n_procs - i) % n_procs];
    const auto& send_buf = send_bufs[i];
    auto& recv_buf = recv_bufs[i];
    size_t send_cnt = send_buf.size();
    size_t recv_cnt = 0;
    MPI_Irecv(
        &recv_cnt, 1, internal::MpiType<size_t>::value, src_proc_id, 0, MPI_COMM_WORLD, &reqs[0]);
    MPI_Isend(
        &send_cnt, 1, internal::MpiType<size_t>::value, dest_proc_id, 0, MPI_COMM_WORLD, &reqs[1]);
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

#pragma omp parallel for schedule(dynamic, 1)
  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    hps::from_string(recv_bufs[i], remote_data[dest_proc_id]);
  }

#pragma omp parallel for schedule(dynamic, 1)
  for (int i = 1; i < n_procs; i++) {
    const int dest_proc_id = shuffled_procs[(shuffled_id + i) % n_procs];
    remote_data[dest_proc_id].for_each_serial(node_handler);
    remote_data[dest_proc_id].clear();
  }

  local_data.sync(reducer);
}

template <class V>
void DistVector<V>::for_each(const std::function<void(const size_t key, V& value)>& handler) {
  const size_t n_procs_u = n_procs;
  const size_t proc_id_u = proc_id;

  const auto& local_handler = [&](const size_t local_key, V& value) {
    const size_t key = local_key * n_procs_u + proc_id_u;
    if (key >= n) return;
    handler(key, value);
  };

  local_data.for_each(local_handler);
}

template <class V>
std::vector<V> DistVector<V>::top_k(
    const size_t k, const std::function<bool(const V&, const V&)>& compare) {
  std::vector<V> local_top_k = local_data.top_k(k, compare);

  std::string msg_buf;
  std::vector<V> partner_top_k;

  int step = 1;
  const int BUF_SIZE = 1 << 20;
  char msg_buf_char[BUF_SIZE];
  size_t msg_size = 0;
  while (step < n_procs) {
    if ((proc_id & (step >> 1)) != 0) break;

    bool is_receiver = (proc_id & step) == 0;
    size_t pos = 0;
    partner_top_k.clear();
    msg_buf.clear();

    if (is_receiver && proc_id + step < n_procs) {
      // Get size.
      MPI_Recv(
          &msg_size,
          1,
          internal::MpiType<size_t>::value,
          proc_id + step,
          0,
          MPI_COMM_WORLD,
          MPI_STATUS_IGNORE);
      msg_buf.reserve(msg_size);

      // Get data.
      while (pos + BUF_SIZE <= msg_size) {
        MPI_Recv(
            msg_buf_char, BUF_SIZE, MPI_CHAR, proc_id + step, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        msg_buf.append(msg_buf_char, BUF_SIZE);
        pos += BUF_SIZE;
      }
      MPI_Recv(
          msg_buf_char,
          msg_size - pos,
          MPI_CHAR,
          proc_id + step,
          0,
          MPI_COMM_WORLD,
          MPI_STATUS_IGNORE);
      msg_buf.append(msg_buf_char, msg_size - pos);

      // Parse data.
      hps::from_string(msg_buf, partner_top_k);

      // Merge.
      std::vector<V> local_top_k_new;
      local_top_k_new.reserve(k);
      size_t pos_local = 0;
      size_t pos_partner = 0;
      size_t pos_new = 0;
      while (pos_new < k && pos_local < local_top_k.size() && pos_partner < partner_top_k.size()) {
        if (compare(local_top_k[pos_local], partner_top_k[pos_partner])) {
          local_top_k_new.push_back(local_top_k[pos_local]);
          pos_local++;
        } else {
          local_top_k_new.push_back(partner_top_k[pos_partner]);
          pos_partner++;
        }
        pos_new++;
      }
      while (pos_new < k && pos_local < local_top_k.size()) {
        local_top_k_new.push_back(local_top_k[pos_local]);
        pos_local++;
        pos_new++;
      }
      while (pos_new < k && pos_partner < partner_top_k.size()) {
        local_top_k_new.push_back(partner_top_k[pos_partner]);
        pos_partner++;
        pos_new++;
      }
      local_top_k = local_top_k_new;
    } else if (!is_receiver) {
      // Serialize data.
      hps::to_string(local_top_k, msg_buf);

      // Send size.
      msg_size = msg_buf.size();
      MPI_Send(&msg_size, 1, internal::MpiType<size_t>::value, proc_id - step, 0, MPI_COMM_WORLD);

      // Send data.
      while (pos + BUF_SIZE <= msg_size) {
        msg_buf.copy(msg_buf_char, BUF_SIZE, pos);
        MPI_Send(msg_buf_char, BUF_SIZE, MPI_CHAR, proc_id - step, 0, MPI_COMM_WORLD);
        pos += BUF_SIZE;
      }
      msg_buf.copy(msg_buf_char, msg_size - pos, pos);
      MPI_Send(msg_buf_char, msg_size - pos, MPI_CHAR, proc_id - step, 0, MPI_COMM_WORLD);
    }

    step <<= 1;
  }

  blaze::broadcast(local_top_k);

  return local_top_k;
}

}  // namespace blaze
