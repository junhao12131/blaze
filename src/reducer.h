#pragma once

#include <functional>

namespace blaze {

template <class T>
class Reducer {
 public:
  static void keep(T&, const T&) {}

  static void overwrite(T& t1, const T& t2) { t1 = t2; }

  static void sum(T& t1, const T& t2) { t1 += t2; }

  static void min(T& t1, const T& t2) {
    if (t1 > t2) t1 = t2;
  }

  static void max(T& t1, const T& t2) {
    if (t1 < t2) t1 = t2;
  }
};
}  // namespace blaze
