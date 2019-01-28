#ifndef BLAZE_DIST_FILE_H_
#define BLAZE_DIST_FILE_H_

#include <string>

#include "dist_vector.h"
#include "internal/mpi_util.h"

namespace blaze {

DistVector<std::string> load_file(const std::string& filename) {
  // Open file.
  int error;
  const size_t TRUNK_SIZE = 1 << 20;
  char buffer[TRUNK_SIZE];
  MPI_File file;
  error = MPI_File_open(MPI_COMM_WORLD, filename.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &file);
  if (error) throw std::runtime_error("Cannot open file");

  // Load file.
  MPI_Offset size;
  MPI_File_get_size(file, &size);
  MPI_Status status;
  std::vector<std::string> lines;
  std::string line;
  size_t line_index = 0;
  const size_t n_procs_u = internal::MpiUtil::get_n_procs();
  const size_t proc_id_u = internal::MpiUtil::get_proc_id();
  bool is_local = proc_id_u == 0;
  while (size > 0) {
    size_t loop_size = std::min(TRUNK_SIZE, static_cast<size_t>(size));
    MPI_File_read_all(file, buffer, loop_size, MPI_CHAR, &status);
    size -= loop_size;
    for (size_t i = 0; i < loop_size; i++) {
      if (buffer[i] == '\n') {
        if (is_local) lines.push_back(line);
        line.clear();
        line_index++;
        is_local = line_index % n_procs_u == proc_id_u;
      } else {
        line.push_back(buffer[i]);
      }
    }
  }
  if (!line.empty()) {
    if (is_local) lines.push_back(line);
    line_index++;
  }

  DistVector<std::string> output(line_index);
  for (size_t i = 0; i < lines.size(); i++) {
    size_t loop_line_index = i * n_procs_u + proc_id_u;
    output.async_set(loop_line_index, std::move(lines[i]));
  }

  MPI_File_close(&file);

  return output;
}
}

#endif
