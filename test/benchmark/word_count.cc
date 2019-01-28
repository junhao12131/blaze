#include "../src/dist_hash_map.h"

#include <gtest/gtest.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "../src/mapreduce.h"

TEST(BenchmarkTest, WordCount) {
  using namespace std::chrono;
  std::ifstream file("test/benchmark/data/word_count_data.txt");
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(file, line)) {
    lines.push_back(line);
  }

  const auto& line_mapper = [&](
      const size_t i, const std::function<void(const std::string&, size_t)>& emit) {
    const auto& line = lines[i];
    std::string word;
    for (char c : line) {
      if (c != ' ') {
        word.push_back(c);
      } else if (!word.empty()) {
        emit(word, 1);
        word.clear();
      }
    }
    if (!word.empty()) {
      emit(word, 1);
    }
  };

  blaze::DistRange<size_t> lines_range(0, lines.size());
  auto start = steady_clock::now();
  for (int i = 0; i <= 3; i++) {
    auto it_start = steady_clock::now();
    blaze::DistHashMap<std::string, size_t> dm;
    blaze::mapreduce<size_t, std::string, size_t>(
        lines_range, line_mapper, blaze::Reducer<size_t>::sum, dm);
    auto it_end = steady_clock::now();
    std::cout << duration_cast<milliseconds>(it_end - it_start).count() << " ms\n";
    std::cout << dm.get_n_keys() << std::endl;
  }
  auto end = steady_clock::now();
  std::cout << duration_cast<milliseconds>(end - start).count() << " ms\n";
}
