# Blaze
High Performance MapReduce-First Cluster Computing Library

[![Build Status](https://travis-ci.org/junhao12131/blaze.svg?branch=master)](https://travis-ci.org/junhao12131/blaze)
[![C++ Version](https://img.shields.io/badge/c%2B%2B-%3E%3D_14-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B14)

This library provides a set of utility functions and distributed data containers for high performance cluster computing.
We provide a highly-optimized MapReduce function to process and convert one data container to another.
This MapReduce function is usually several times faster than Aparch Spark at the time of this update.
For most parallel algorithms, users only need to write a few mapper functions and they will get similar performance as hand-optimized code written with raw sockets and threads.
We also provide some additional features to complement MapReduce, such as loading files to distributed data containers, thread-safe random number generators, and functions for converting distributed containers to and from C++ standard containers, etc.

## Example
In this section, we present two examples to illustrate the usage of Blaze.
The complete code of these examples can be found in the `example` folder.
For your convenience, we also provide the corresponding `Makefile`.

### Word Count
In this example, we build a distributed hash map of word occurrences`<std::string, size_t>` and output the number of unique words.
```C++
auto lines = blaze::util::load_file("filepath...");  // Source
const auto& mapper = [&](const size_t, const std::string& line, const auto& emit) {
  // First argument is line_id, which is not needed.
  std::stringstream ss(line);
  std::string word;
  while (getline(ss, word, ' ')) emit(word, 1);  // Split line into words.
};
blaze::DistHashMap<std::string, size_t> words;  // Target
blaze::mapreduce<std::string, std::string, size_t>(lines, mapper, "sum", words);
std::cout << words.size() << std::endl;
```

### Pi Estimate
In this example, we estimate π using the Monte Carlo method.
```C++
const size_t N_SAMPLES = 1000000;
blaze::DistRange<size_t> samples(0, N_SAMPLES);
const auto& mapper = [&](const size_t, const auto& emit) {
  double x = blaze::random::uniform();
  double y = blaze::random::uniform();
  if (x * x + y * y < 1) emit(0, 1);
};
std::vector<size_t> count(1);  // {0}
blaze::mapreduce<size_t, size_t>(samples, mapper, "sum", count);
std::cout << 4.0 * count[0] / N_SAMPLES << std::endl;
```
For conventional MapReduce implementations, this use case is usually not efficient and consumes large amount of memory.
However, with `blaze`, the above code will have similar memory consumption and achieve similar performance as a carefully-optimized implementation using hand-coded parallel for loops.

## Benchmark
We present two benchmarks here:
one is with Apache Spark on AWS to show its superior performance on a multi-machine cluster;
the other one is with hand-optimized parallel for loops to illustrate that Blaze can approach similar performance as hand-optimized code;

### Word Frequency Count (on AWS)
The task counts the number of occurrences of each unique word from a text file, which contains about 0.4 billion words in total.
The performance of Blaze is compared to Apache Spark (from AWS EMR 5.20.0) on several AWS EC2 r5.xlarge instances.

![Word Frequency Count Performance](https://raw.githubusercontent.com/junhao12131/blaze/master/test/benchmark/plot/wordcount_speed.png)

### Pi Estimation (local)
This task compares the performance and source lines of code (SLOC) of the Pi estimation between a Blaze MapReduce implementation and a hand-optimized implementation.

| Samples | Blaze MapReduce | MPI+OpenMP |
| --- | :---: | :---: |
| **10M** | 12 | 23 |
| **100M** | 15 | 25 |
| **1G** | 13 | 20 |
| **SLOC** | 7 | 16 |
