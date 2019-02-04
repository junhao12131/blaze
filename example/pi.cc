#include <blaze/blaze.h>
#include <iostream>

int main(int argc, char** argv) {
  blaze::util::init(argc, argv);
  const size_t N_SAMPLES = 1000000;
  blaze::DistRange<size_t> samples(0, N_SAMPLES);
  const auto& mapper = [&](const size_t, const auto& emit) {
    double x = blaze::random::uniform();
    double y = blaze::random::uniform();
    if (x * x + y * y < 1) emit(0, 1);
  };
  std::vector<size_t> count(1);
  blaze::mapreduce<size_t, size_t>(samples, mapper, "sum", count);
  std::cout << 4.0 * count[0] / N_SAMPLES << std::endl;
}
