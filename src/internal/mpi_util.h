#pragma once

#include <mpi.h>

namespace blaze {
namespace internal {

class MpiUtil {
 public:
  static int get_n_procs() { return get_instance().n_procs; }

  static int get_proc_id() { return get_instance().proc_id; }

  static bool is_master() { return get_proc_id() == 0; }

 private:
  int n_procs;

  int proc_id;

  MpiUtil() {
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
  }

  static MpiUtil get_instance() {
    static MpiUtil instance;
    return instance;
  }
};
}  // namespace internal
}  // namespace blaze
