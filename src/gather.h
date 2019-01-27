#ifndef BLAZE_GATHER_H_
#define BLAZE_GATHER_H_

#include <hps/src/hps.h>
#include <algorithm>
#include <cassert>
#include <vector>
#include "internal/mpi_type.h"
#include "internal/mpi_util.h"

namespace blaze {

// Gather objects of any type and any size.
template <class T>
std::vector<T> gather(T& t) {
  const int n_procs = internal::MpiUtil::get_n_procs();
  std::vector<T> res(n_procs);
  if (n_procs == 1) {
    res[0] = t;
    return res;
  }

  // Gather message sizes.
  std::string msg_buf = hps::to_string(t);
  const size_t msg_size = msg_buf.size();
  std::vector<size_t> msg_sizes(n_procs, 0);
  const MPI_Datatype size_t_mpi = internal::MpiType<size_t>::value;
  MPI_Allgather(&msg_size, 1, size_t_mpi, msg_sizes.data(), 1, size_t_mpi, MPI_COMM_WORLD);

  // Gather message.
  const size_t max_msg_size = *std::max_element(msg_sizes.begin(), msg_sizes.end());
  msg_buf.reserve(max_msg_size);
  std::vector<std::string> recv_buf(n_procs);
  size_t pos = 0;
  const int BUF_SIZE = 1 << 20;
  char msg_buf_char[BUF_SIZE];
  assert(BUF_SIZE > n_procs);
  int buf_size_per = BUF_SIZE / n_procs;
  const int proc_id = internal::MpiUtil::get_proc_id();
  res[proc_id] = t;

  for (int i = 0; i < n_procs; i++) recv_buf[i].reserve(max_msg_size);
  while (pos < max_msg_size) {
    int gather_size = buf_size_per;
    if (pos + buf_size_per > max_msg_size) {
      gather_size = max_msg_size - pos;
    }
    MPI_Allgather(
        msg_buf.data() + pos,
        gather_size,
        MPI_CHAR,
        msg_buf_char,
        buf_size_per,
        MPI_CHAR,
        MPI_COMM_WORLD);
    for (int i = 0; i < n_procs; i++) {
      if (i == proc_id) continue;
      recv_buf[i].append(msg_buf_char + i * buf_size_per, gather_size);
    }
    pos += gather_size;
  }

  for (int i = 0; i < n_procs; i++) {
    if (i == proc_id) continue;
    hps::from_string(recv_buf[i], res[i]);
  }

  return res;
}
}  // namespace blaze

#endif
