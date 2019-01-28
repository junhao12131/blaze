# Blaze
High Performance MapReduce-First Cluster Computing Library

[![Build Status](https://travis-ci.org/junhao12131/blaze.svg?branch=master)](https://travis-ci.org/junhao12131/blaze)
[![C++ Version](https://img.shields.io/badge/c%2B%2B-%3E%3D_14-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B14)

This library provides a set of utility functions and distributed data containers for high performance cluster computing.
We provide MapReduce functions to process and convert one data container to another.
These functions are optimized from ground up and are usually several times faster than Aparch Spark at the time of this update.
For most parallel algorithms, users only need to write a few mapper functions and they will get similar performance as hand-optimized code written with raw sockets and threads.
We also provide some additional features to complement MapReduce, such as loading files to distributed data containers, converting to and from C++ standard containers, etc.

## Example
### Word Count
In this example, we build a distributed hash map of word occurrences`<std::string, size_t>` and output the number of unique words.
```C++
auto lines = blaze::util::load_file("filepath...");  // Source
const auto& mapper = [&](const size_t, const std::string& line, const auto& emit) {
  // First argument is line_id, which is not needed.
  std::stringstream ss(line);
  std::string word;
  while (getline(ss, word, ' ')) emit(word, 1);  // Split into tokens.
};
blaze::DistHashMap<std::string, size_t> words;  // Target
blaze::mapreduce<std::string, std::string, size_t>(lines, mapper, "sum", words);
std::cout << words.size() << std::endl;
```

### Pi Estimate
In this example, we estimate π using the Monte Carlo method.
```C++
double thread_safe_rand() {
  // The default random number generator is not thread safe.
  static thread_local std::mt19937 generator(
      std::chrono::system_clock::now().time_since_epoch().count());
  std::uniform_real_distribution<double> distribution;
  return distribution(generator);
}

const size_t N_SAMPLES = 1000000;
blaze::DistRange<size_t> samples(0, N_SAMPLES);
const auto& mapper = [&](const size_t, const auto& emit) {
  double x = thread_safe_rand();
  double y = thread_safe_rand();
  if (x * x + y * y < 1) emit(0, 1);
};
std::vector<size_t> count(1);  // {0}
blaze::mapreduce<size_t, size_t>(samples, mapper, "sum", count);
std::cout << 4.0 * count[0] / N_SAMPLES << std::endl;
```
For conventional MapReduce implementations, this use case is usually not efficient and consumes large amount of memory.
However, with `blaze`, the above code will have similar memory consumption and achieve similar performance as a carefully-optimized implementation using hand-coded parallel for loops.

## Benchmarks

## APIs
