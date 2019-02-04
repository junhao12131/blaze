#ifndef BLAZE_RANDOM_H_
#define BLAZE_RANDOM_H_

#include <chrono>
#include <random>

namespace blaze {
class random {
 public:
  static double uniform(double low = 0, double high = 1) {
    static thread_local std::mt19937 generator(
        std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<double> distribution(low, high);
    return distribution(generator);
  }

  static int uniform_int(int low = 0, int high = 1) {
    static thread_local std::mt19937 generator(
        std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(low, high);
    return distribution(generator);
  }

  static double normal(double mu = 0, double sigma = 1) {
    static thread_local std::mt19937 generator(
        std::chrono::system_clock::now().time_since_epoch().count());
    std::normal_distribution<double> distribution(mu, sigma);
    return distribution(generator);
  }
};
}

#endif
