#include <hps/src/hps.h>
#include "internal/mpi_type.h"
#include "internal/mpi_util.h"

namespace blaze {

// Broadcast objects of any type and any size.
template <class T>
void broadcast(T& t, const int root = 0) {
  const int n_procs = internal::MpiUtil::get_n_procs();
  if (n_procs == 1) return;

  size_t msg_size;
  const int BUF_SIZE = 1 << 20;
  char msg_buf_char[BUF_SIZE];
  std::string msg_buf;
  const int proc_id = internal::MpiUtil::get_proc_id();
  const bool is_master = (proc_id == root);

  // Broadcast size.
  size_t pos = 0;
  if (is_master) {
    hps::to_string(t, msg_buf);
    msg_size = msg_buf.size();
    MPI_Bcast(&msg_size, 1, internal::MpiType<size_t>::value, root, MPI_COMM_WORLD);
    while (pos + BUF_SIZE <= msg_size) {
      msg_buf.copy(msg_buf_char, BUF_SIZE, pos);
      MPI_Bcast(msg_buf_char, BUF_SIZE, MPI_CHAR, root, MPI_COMM_WORLD);
      pos += BUF_SIZE;
    }
    msg_buf.copy(msg_buf_char, msg_size - pos, pos);
    MPI_Bcast(msg_buf_char, msg_size - pos, MPI_CHAR, root, MPI_COMM_WORLD);
  } else {
    MPI_Bcast(&msg_size, 1, internal::MpiType<size_t>::value, root, MPI_COMM_WORLD);
    msg_buf.reserve(msg_size);
    while (pos + BUF_SIZE <= msg_size) {
      MPI_Bcast(msg_buf_char, BUF_SIZE, MPI_CHAR, root, MPI_COMM_WORLD);
      msg_buf.append(msg_buf_char, BUF_SIZE);
      pos += BUF_SIZE;
    }
    MPI_Bcast(msg_buf_char, msg_size - pos, MPI_CHAR, root, MPI_COMM_WORLD);
    msg_buf.append(msg_buf_char, msg_size - pos);
    hps::from_string(msg_buf, t);
  }
}
}  // namespace blaze
