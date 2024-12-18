#include <chrono>
#include <cppitertools/itertools.hpp>
#include <gtest/gtest.h>
#include "../src/common_tool.h"
using namespace std;
// Demonstrate some basic assertions.
TEST(CommonTest, countStringTokenTest)
{
  init_log();
  int countSize = countStringToken("Hello\n   childer, I wish   you are happy!");
  std::cout << "size:   " << countSize << std::endl;
}

TEST(CommonTest, parameterCombination)
{
  std::unordered_map<string, vector<string>> parameters;
  parameters["max_depth"] = {"3", "5", "7", "9"};
  parameters["min_child_weight"] = {"1", "3", "5", "7"};
  parameters["n_estimators"] = {"5", "10", "20"};
  parameters["learning_rate"] = {"0.01", "0.05", "0.1"};
  for (auto &&[a, b, c] : iter::product(parameters["max_depth"], parameters["min_child_weight"], parameters["n_estimators"]))
  {
    std::cout << a << ", " << b << "," << c << std::endl;
  }
}

TEST(CommonTest, getEnvTest)
{
  // --gtest_filter=CommonTest.getEnvTest
  if (std::getenv("TEST_ENV"))
  {
    std::cout << "TEST_ENV exist " << std::getenv("TEST_ENV") << std::endl;
  }
  else if (std::getenv("TEST_ENV") == nullptr)
  {
    std::cout << "TEST_ENV not exist " << std::endl;
  }
}

TEST(CommonTest, timeStampTest)
{
  // --gtest_filter=CommonTest.timeStampTest
  const auto p1 = std::chrono::system_clock::now();

  std::cout << "seconds since epoch: "
            << std::chrono::duration_cast<std::chrono::seconds>(
                   p1.time_since_epoch())
                   .count()
            << '\n';
  std::cout << getTimeStampNow() << std::endl;
}

TEST(CommonTest, partitionChunkTest)
{
  // --gtest_filter=CommonTest.partitionChunkTest
  std::vector<std::string> names = {"Kenneth", "Jennifer", "Lynn", "Sole", "Zaynor"};

  // Split into sub groups
  auto namesPartition = partitionChunk(names.begin(), names.end(), 2);

  // Display grouped batches
  for (unsigned batchCount = 0; batchCount < namesPartition.size(); ++batchCount)
  {
    auto batch = namesPartition[batchCount];

    std::cout << "Batch #" << batchCount + 1 << std::endl;
    for (const auto &item : batch)
    {
      std::cout << "  Item: " << item << std::endl;
    }
  }
}

TEST(CosineSimilarityTest, BasicTest)
{
  // --gtest_filter=CosineSimilarityTest.BasicTest
  VectorXd A(3);
  A << 1.0, 2.0, 3.0;

  VectorXd B(3);
  B << 4.0, 5.0, 6.0;

  double result = eigen_cosine_similarity(A, B);
  double expected_result = 0.9746318461970762; // Expected value, can be calculated manually

  // Use Google Test assertions to check if the result meets the expectation
  EXPECT_NEAR(result, expected_result, 1e-6);
}

// Test 2: The cosine similarity of two identical vectors should be 1
TEST(CosineSimilarityTest, IdenticalVectors)
{
  // --gtest_filter=CosineSimilarityTest.IdenticalVectors

  VectorXd A(3);
  A << 1.0, 1.0, 1.0;

  VectorXd B(3);
  B << 1.0, 1.0, 1.0;

  double result = eigen_cosine_similarity(A, B);
  double expected_result = 1.0;

  EXPECT_NEAR(result, expected_result, 1e-6);
}

TEST(CosineSimilarityTest, OppositeVectors)
{
  // --gtest_filter=CosineSimilarityTest.OppositeVectors

  VectorXd A(3);
  A << 1.0, 1.0, 1.0;

  VectorXd B(3);
  B << -1.0, -1.0, -1.0;

  double result = eigen_cosine_similarity(A, B);
  double expected_result = -1.0;

  EXPECT_NEAR(result, expected_result, 1e-6);
}

// Test 3: The cosine similarity of two orthogonal vectors should be 0
TEST(CosineSimilarityTest, OrthogonalVectors)
{
  //  --gtest_filter=CosineSimilarityTest.OrthogonalVectors
  VectorXd A(3);
  A << 1.0, 0.0, 0.0;

  VectorXd B(3);
  B << 0.0, 1.0, 0.0;

  double result = eigen_cosine_similarity(A, B);
  double expected_result = 0;

  EXPECT_NEAR(result, expected_result, 1e-6);
}

// Test 4: The cosine similarity of a zero vector with any other vector should be NaN or undefined
TEST(CosineSimilarityTest, ZeroVector)
{
  //  --gtest_filter=CosineSimilarityTest.ZeroVector

  VectorXd A(3);
  A << 0.0, 0.0, 0.0;

  VectorXd B(3);
  B << 1.0, 2.0, 3.0;

  // When cosine similarity is NaN, check if the result is NaN
  double result = eigen_cosine_similarity(A, B);
  EXPECT_TRUE(std::isnan(result)); // Check if the result is NaN
}

// Test 5: Edge case, empty vectors (although usually not allowed, but can test the behavior)
TEST(CosineSimilarityTest, EmptyVector)
{
  //  --gtest_filter=CosineSimilarityTest.EmptyVector
  VectorXd A(0);
  VectorXd B(0);

  double result = eigen_cosine_similarity(A, B);

  // If empty vectors are allowed, the result might be NaN, or you need to modify the function to throw an exception
  EXPECT_TRUE(std::isnan(result)); // Here we assume we return NaN
}