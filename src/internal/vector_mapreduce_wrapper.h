#ifndef BLAZE_VECTOR_UTIL_H_
#define BLAZE_VECTOR_UTIL_H_

#include <vector>
#include <string>
#include <functional>

#include "mpi_util.h"
#include "mpi_type.h"

namespace blaze {
namespace internal {

template <class VD>
class VectorMapreduceWrapper {
 public:
  VectorMapreduceWrapper(std::vector<VD>& target);

  void async_set(
      const size_t key, const VD& value, const std::function<void(VD&, const VD&)>& reducer);

  void sync(const std::function<void(VD&, const VD&)>& reducer);

  void sync(const std::string& reducer);

 private:
  size_t n_keys;

  std::vector<std::vector<VD>> res_threads;

  std::vector<VD>* target_ptr;

  std::vector<VD> res_local;

  void sync_local(const std::function<void(VD&, const VD&)>& reducer);
};

template <class VD>
VectorMapreduceWrapper<VD>::VectorMapreduceWrapper(std::vector<VD>& target) {
  n_keys = target.size();
  const int n_threads = omp_get_max_threads();

  res_threads.resize(n_threads);
  const VD default_value = VD();
  for (int i = 0; i < n_threads; i++) {
    res_threads[i].assign(n_keys, default_value);
  }

  target_ptr = &target;
}

template <class VD>
void VectorMapreduceWrapper<VD>::async_set(
    const size_t key, const VD& value, const std::function<void(VD&, const VD&)>& reducer) {
  const int thread_id = omp_get_thread_num();
  reducer(res_threads[thread_id][key], value);
}

template <class VD>
void VectorMapreduceWrapper<VD>::sync(const std::function<void(VD&, const VD&)>& reducer) {
  sync_local(reducer);

  std::vector<VD> res_remote;

  // Cross-node tree reduce.
  int step = 1;
  const int n_procs = blaze::internal::MpiUtil::get_n_procs();
  const int proc_id = blaze::internal::MpiUtil::get_proc_id();
  const int BUF_SIZE = 1 << 20;
  std::string msg_buf;
  char msg_buf_char[BUF_SIZE];
  size_t msg_size = 0;
  while (step < n_procs) {
    if ((proc_id & (step >> 1)) != 0) break;
    bool is_receiver = (proc_id & step) == 0;
    size_t pos = 0;
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
      if (pos < msg_size) {
        MPI_Recv(
            msg_buf_char,
            msg_size - pos,
            MPI_CHAR,
            proc_id + step,
            0,
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);
        msg_buf.append(msg_buf_char, msg_size - pos);
      }

      // Parse data.
      hps::from_string(msg_buf, res_remote);

      // Reduce.
      for (size_t i = 0; i < n_keys; i++) reducer(res_local[i], res_remote[i]);
    } else {
      hps::to_string(res_local, msg_buf);

      // Send size.
      msg_size = msg_buf.size();
      MPI_Send(&msg_size, 1, internal::MpiType<size_t>::value, proc_id - step, 0, MPI_COMM_WORLD);

      // Send data.
      while (pos + BUF_SIZE <= msg_size) {
        msg_buf.copy(msg_buf_char, BUF_SIZE, pos);
        MPI_Send(msg_buf_char, BUF_SIZE, MPI_CHAR, proc_id - step, 0, MPI_COMM_WORLD);
        pos += BUF_SIZE;
      }
      if (pos < msg_size) {
        msg_buf.copy(msg_buf_char, msg_size - pos, pos);
        MPI_Send(msg_buf_char, msg_size - pos, MPI_CHAR, proc_id - step, 0, MPI_COMM_WORLD);
      }
    }

    step <<= 1;
  }

  blaze::broadcast(res_local);

#pragma omp parallel for schedule(static)
  for (size_t i = 0; i < n_keys; i++) {
    reducer(target_ptr->at(i), res_local[i]);
  }
}

template <class VD>
void VectorMapreduceWrapper<VD>::sync(const std::string& reducer) {
  const auto& reducer_func = internal::MapreduceUtil::get_reducer_func<VD>(reducer);

  sync_local(reducer_func);

  std::vector<VD> res(n_keys);

  // Cross-node tree reduce.
  MPI_Allreduce(
      res_local.data(),
      res.data(),
      n_keys,
      MpiType<VD>::value,
      MapreduceUtil::get_mpi_op(reducer),
      MPI_COMM_WORLD);

#pragma omp parallel for schedule(static)
  for (size_t i = 0; i < n_keys; i++) {
    reducer_func(target_ptr->at(i), res[i]);
  }
}

template <class VD>
void VectorMapreduceWrapper<VD>::sync_local(const std::function<void(VD&, const VD&)>& reducer) {
  // Node reduce.
  int step = 1;
  const int n_threads = omp_get_max_threads();
  while (step < n_threads) {
    int i_end = n_threads - step;
    int i_step = step << 1;
#pragma omp parallel for schedule(static, 1)
    for (int i = 0; i < i_end; i += i_step) {
      for (size_t j = 0; j < n_keys; j++) {
        reducer(res_threads[i][j], res_threads[i + step][j]);
      }
    }
    step <<= 1;
  }

  res_local = res_threads[0];

  const VD default_value = VD();
  for (int i = 0; i < n_threads; i++) {
    res_threads[i].assign(n_keys, default_value);
  }
}
}
}

#endif
