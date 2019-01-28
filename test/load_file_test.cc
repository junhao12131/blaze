#include "../src/load_file.h"

#include <gtest/gtest.h>

#include "../src/collect.h"
#include "../src/mapreduce.h"

TEST(LoadFileTest, LoadAndSumNumbers) {
  auto dist_numbers = blaze::load_file("test/numbers.txt");
  const auto& number_mapper = [&](
      const size_t, const std::string& value, const std::function<void(size_t, const size_t&)>& emit) {
    emit(0, std::stoull(value));
  };
  std::vector<size_t> result_vec(1);
  blaze::mapreduce<std::string, size_t>(dist_numbers, number_mapper, "sum", result_vec);

  EXPECT_EQ(result_vec[0], 255);
}
