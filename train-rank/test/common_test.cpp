#include <chrono>
#include <cppitertools/itertools.hpp>
#include <gtest/gtest.h>
#include "../src/common_tool.h"
using namespace std;
// Demonstrate some basic assertions.
TEST(CommonTest, countStringTokenTest) {
  init_log();
  int countSize = countStringToken("Hello\n   childer, I wish   you are happy!");
  std::cout << "size:   " << countSize << std::endl;
}

TEST(CommonTest, parameterCombination) {
        std::unordered_map<string,vector<string>> parameters;
        parameters["max_depth"] = {"3","5","7","9"};
        parameters["min_child_weight"] = {"1","3","5","7"};
        parameters["n_estimators"] = {"5","10","20"};
        parameters["learning_rate"] = {"0.01","0.05","0.1"};
        for (auto&& [a, b,c] : iter::product(parameters["max_depth"],parameters["min_child_weight"],parameters["n_estimators"])) {
            std::cout << a << ", " << b << "," << c << std::endl;
        }
}

TEST(CommonTest,getEnvTest) {
   // --gtest_filter=CommonTest.getEnvTest
  if(std::getenv("TEST_ENV")) {
    std::cout << "TEST_ENV exist " << std::getenv("TEST_ENV") << std::endl;
  }else if(std::getenv("TEST_ENV")==nullptr){
    std::cout << "TEST_ENV not exist " << std::endl;
  }

}

TEST(CommonTest, timeStampTest) {
  // --gtest_filter=CommonTest.timeStampTest
    const auto p1 = std::chrono::system_clock::now();

    std::cout << "seconds since epoch: "
              << std::chrono::duration_cast<std::chrono::seconds>(
                   p1.time_since_epoch()).count() << '\n';
    std::cout << getTimeStampNow() << std::endl;
}

TEST(CommonTest, partitionChunkTest) {
      // --gtest_filter=CommonTest.partitionChunkTest
      std::vector<std::string> names = { "Kenneth", "Jennifer", "Lynn", "Sole", "Zaynor" };
      
      // Split into sub groups
      auto namesPartition = partitionChunk(names.begin(), names.end(), 2);
      
      // Display grouped batches
      for (unsigned batchCount = 0; batchCount < namesPartition.size(); ++batchCount) {
          auto batch = namesPartition[batchCount];
      
          std::cout << "Batch #" << batchCount + 1 << std::endl;
          for (const auto& item : batch) {
              std::cout << "  Item: " << item << std::endl;
          }
      }
 
}