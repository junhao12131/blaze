#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mpi.h>
#include <iostream>

GTEST_API_ int main(int argc, char** argv) {
  MPI_Init(nullptr, nullptr);
  int proc_id;
  MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
  char filename[16];
  sprintf(filename, "proc%d.log", proc_id);
  if (proc_id != 0) freopen(filename, "w", stdout);

  std::cout << "Running main() from gmock_main_mpi.cc\n";
  testing::InitGoogleMock(&argc, argv);
  const auto& res = RUN_ALL_TESTS();

  MPI_Finalize();
  return res;
}
