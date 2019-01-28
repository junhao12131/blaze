#include <iostream>
#include <string>
#include <gtest/gtest.h>

#include "../../blaze.h"

TEST(ExampleTest, WordCount) { 
  auto lines = blaze::util::load_file("test/example/word_count_example.txt");
  const auto& mapper = [&](const size_t, const std::string& line, const auto& emit) {
    std::stringstream ss(line);
    std::string word;
    while (getline(ss, word, ' ')) emit(word, 1);
  };
  blaze::DistHashMap<std::string, size_t> words;
  blaze::mapreduce<std::string, std::string, size_t>(lines, mapper, "sum", words);
  std::cout << words.size() << std::endl;
}
