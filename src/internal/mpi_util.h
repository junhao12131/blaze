#ifndef BLAZE_INTERNAL_MPI_UTIL_H_
#define BLAZE_INTERNAL_MPI_UTIL_H_

#include <mpi.h>
#include <algorithm>
#include <stdexcept>

namespace blaze {
namespace internal {

class MpiUtil {
 public:
  static int get_n_procs() { return get_instance().n_procs; }

  static int get_proc_id() { return get_instance().proc_id; }

  static std::vector<int> generate_shuffled_procs() {
    int n_procs = get_n_procs();
    std::vector<int> res(n_procs);

    if (is_master()) {
      for (int i = 0; i < n_procs; i++) res[i] = i;
      std::random_shuffle(res.begin(), res.end());
    }
    MPI_Bcast(res.data(), n_procs, MPI_INT, 0, MPI_COMM_WORLD);

    return res;
  }

  static int get_shuffled_id(const std::vector<int>& shuffled_procs) {
    int n_procs = get_n_procs();
    int proc_id = get_proc_id();

    for (int i = 0; i < n_procs; i++) {
      if (shuffled_procs[i] == proc_id) return i;
    }

    throw std::runtime_error("proc id does not exist in shuffled procs.");
  }

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

#endif
