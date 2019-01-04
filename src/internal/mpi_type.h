#pragma once

#include <mpi.h>

namespace blaze {
namespace internal {

template <class T>
struct MpiType {};

template <>
struct MpiType<char> {
  constexpr static MPI_Datatype value = MPI_CHAR;
};

template <>
struct MpiType<short> {
  constexpr static MPI_Datatype value = MPI_SHORT;
};

template <>
struct MpiType<int> {
  constexpr static MPI_Datatype value = MPI_INT;
};

template <>
struct MpiType<long> {
  constexpr static MPI_Datatype value = MPI_LONG;
};

template <>
struct MpiType<long long> {
  constexpr static MPI_Datatype value = MPI_LONG_LONG_INT;
};

template <>
struct MpiType<unsigned char> {
  constexpr static MPI_Datatype value = MPI_UNSIGNED_CHAR;
};

template <>
struct MpiType<unsigned short> {
  constexpr static MPI_Datatype value = MPI_UNSIGNED_SHORT;
};

template <>
struct MpiType<unsigned> {
  constexpr static MPI_Datatype value = MPI_UNSIGNED;
};

template <>
struct MpiType<unsigned long> {
  constexpr static MPI_Datatype value = MPI_UNSIGNED_LONG;
};

template <>
struct MpiType<unsigned long long> {
  constexpr static MPI_Datatype value = MPI_UNSIGNED_LONG_LONG;
};

template <>
struct MpiType<float> {
  constexpr static MPI_Datatype value = MPI_FLOAT;
};

template <>
struct MpiType<double> {
  constexpr static MPI_Datatype value = MPI_DOUBLE;
};

template <>
struct MpiType<long double> {
  constexpr static MPI_Datatype value = MPI_LONG_DOUBLE;
};
};  // namespace internal
};  // namespace blaze
