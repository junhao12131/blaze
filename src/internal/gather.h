#pragma once

#include <hps/src/hps.h>
#include <algorithm>
#include <vector>
#include "internal/mpi_type.h"
#include "internal/mpi_util.h"

namespace blaze {
namespace internal {

// Gather objects of any type and any size.
template <class T>
std::vector<T> gather(T& t) {
  const std::string serialized = hps::to_string(t);
  const size_t count = serialized.size();
  const int n_procs = internal::MpiUtil::get_n_procs();
  std::vector<size_t> counts(n_procs, 0);
  const MPI_Datatype size_t_mpi = internal::MpiType<size_t>::value;
  MPI_Allgather(&count, 1, size_t_mpi, counts.data(), 1, size_t_mpi, MPI_COMM_WORLD);

  const size_t max_count = *std::max_element(counts.begin(), counts.end());
  const size_t BUF_SIZE = 1 << 18;
  char send_buf_char[BUF_SIZE];
  char recv_buf_char[BUF_SIZE * n_procs];
  std::vector<std::string> recv_bufs(n_procs);
  size_t gather_pos = 0;

  while (gather_pos < max_count) {
    const int gather_cnt = gather_pos + BUF_SIZE <= max_count ? BUF_SIZE : max_count - gather_pos;
    MPI_Allgather(send_buf_char, gather_cnt, MPI_CHAR, recv_buf_char, gather_cnt, MPI_CHAR, MPI_COMM_WORLD);
   
  }

  char* buffer_send = const_cast<char*>(serialized.data());
  char* buffer_recv = new char[max_count];
  char* buffer_ptr = nullptr;
  std::vector<T> res(n_procs);
  const int TRUNK_SIZE = 1 << 20;
  const int proc_id = internal::MpiUtil::get_proc_id();
  for (int root = 0; root < n_procs; root++) {
    const bool is_root = (proc_id == root);
    size_t count_root = counts[root];
    if (is_root) {
      buffer_ptr = buffer_send;
    } else {
      buffer_ptr = buffer_recv;
    }
    while (count_root > TRUNK_SIZE) {
      MPI_Bcast(buffer_ptr, TRUNK_SIZE, MPI_CHAR, root, MPI_COMM_WORLD);
      buffer_ptr += TRUNK_SIZE;
      count_root -= TRUNK_SIZE;
    }
    MPI_Bcast(buffer_ptr, count_root, MPI_CHAR, root, MPI_COMM_WORLD);

    if (is_root) {
      hps::from_char_array(buffer_send, res[root]);
    } else {
      hps::from_char_array(buffer_recv, res[root]);
    }
  }
  delete[] buffer_recv;
  return res;
}

}  // namespace internal
}  // namespace blaze
