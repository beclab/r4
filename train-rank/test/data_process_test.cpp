#include "../src/data_process.h"

#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../src/common_tool.h"
#include "../src/easylogging++.h"

using namespace std;
// Demonstrate some basic assertions.
TEST(DataProcessTest, RandomOverResampleTest) {
  init_log();
  vector<string> more1{"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"};
  vector<string> less1{"X", "Y", "Z"};
  RandomOverResample(&more1, &less1);
  EXPECT_EQ(more1.size(), less1.size());
  LOG(INFO) << "less1 size" << less1.size();
  printVector(less1);
  // LOG(INFO) << less1;
  vector<string> more2{"A", "B", "C", "D", "E", "F"};
  vector<string> less2{"X", "Y"};
  RandomOverResample(&less2, &more2);
}

TEST(DataProcessTest, WhetherRealRandomTest) {
  // --gtest_filter=DataProcessTest.WhetherRealRandomTest
  vector<string> more1{"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"};
  vector<string> temp1;
  vector<string> temp2;
  std::sample(more1.begin(), more1.end(), std::back_inserter(temp1), 3,
              std::mt19937{std::random_device{}()});
  std::sample(more1.begin(), more1.end(), std::back_inserter(temp2), 20,
              std::mt19937{std::random_device{}()});
  printVector(temp1);
  printVector(temp2);
}

TEST(DataProcessTest, RandomOverResampleSpecificNumberTest) {
  // --gtest_filter=DataProcessTest.RandomOverResampleSpecificNumberTest
  init_log();
  vector<string> more1{"A", "B", "C", "D", "E", "F"};
  int specific_size = 15;
  RandomOverResampleSpecificNumber(specific_size, &more1);
  EXPECT_EQ(specific_size, more1.size());
  printVector(more1);
}

TEST(DataProcessTest, ConfigureTrainTestWithoutOversampleTest) {
  // --gtest_filter=DataProcessTest.ConfigureTrainTestWithoutOversampleTest
  init_log();
  unordered_set<string> positive{"A", "B", "C", "D", "E", "F", "G", "H",
                                 "I", "J", "K", "L", "M", "N", "O", "P",
                                 "Q", "R", "S", "T", "U", "V", "W"};
  unordered_set<string> negative{"1", "2", "3", "4", "5",
                                 "6", "7", "8", "9", "10"};
  unordered_set<string> train_positive_sample;
  unordered_set<string> train_negative_sample;
  unordered_set<string> test_positive_sample;
  unordered_set<string> test_negative_sample;
  ConfigureTrainTestRatioWithoutOverSample(
      positive, negative, &train_positive_sample, &train_negative_sample,
      &test_positive_sample, &test_negative_sample, 0.8);
  vector<string> intersection_positive;
  std::set_intersection(
      train_positive_sample.begin(), train_positive_sample.end(),
      test_positive_sample.begin(), test_positive_sample.end(),
      std::back_inserter(intersection_positive));
  EXPECT_EQ(intersection_positive.size(), 0);
  EXPECT_EQ(train_positive_sample.size(), int(positive.size() * 0.8));
  EXPECT_EQ(train_negative_sample.size(), int(negative.size() * 0.8));
  vector<string> intersection_negative;
  std::set_intersection(
      train_negative_sample.begin(), train_negative_sample.end(),
      test_negative_sample.begin(), test_negative_sample.end(),
      std::back_inserter(intersection_negative));
  EXPECT_EQ(intersection_positive.size(), 0);
}

TEST(DataProcessTest, ConfigureTrainTestRationTest) {
  init_log();
  unordered_set<string> positive{"A", "B", "C", "D", "E", "F", "G", "H",
                                 "I", "J", "K", "L", "M", "N", "O", "P",
                                 "Q", "R", "S", "T", "U", "V", "W"};
  unordered_set<string> negative{"1", "2", "3", "4", "5",
                                 "6", "7", "8", "9", "10"};

  vector<string> train_sample;
  vector<string> test_sample;
  ConfigureTrainTestRation(positive, negative, &train_sample, &test_sample, 0.8);

  std::unordered_set<string> train_sample_set(train_sample.begin(),
                                              train_sample.end());
  std::unordered_set<string> test_sample_set(test_sample.begin(),
                                             test_sample.end());

  vector<string> intersection;
  std::set_intersection(train_sample_set.begin(), train_sample_set.end(),
                        test_sample_set.begin(), test_sample_set.end(),
                        std::back_inserter(intersection));
  EXPECT_EQ(intersection.size(), 0);

  int train_sample_positive_count = 0;
  int train_sample_negative_count = 0;
  for (auto current_item : train_sample) {
    if (positive.count(current_item)) {
      train_sample_positive_count++;
    }
    if (negative.count(current_item)) {
      train_sample_negative_count++;
    }
  }
  EXPECT_EQ(train_sample_negative_count, train_sample_positive_count);

  int test_sample_positive_count = 0;
  int test_sample_negative_count = 0;
  for (auto current_item : test_sample) {
    if (positive.count(current_item)) {
      test_sample_positive_count++;
    }
    if (negative.count(current_item)) {
      test_sample_negative_count++;
    }
  }
  EXPECT_EQ(test_sample_positive_count, test_sample_negative_count);
}

TEST(DataProcessTest, TokenizerTest) {
  string line = "GeeksForGeeks     is a must try";

  // Vector of string to save tokens
  vector<string> tokens;

  // stringstream class check1
  stringstream check1(line);

  string intermediate;

  // Tokenizing w.r.t. space ' '
  while (getline(check1, intermediate, ' ')) {
    if (isStringEmptyOrWhitespace(intermediate)) {
      continue;
    }
    tokens.push_back(intermediate);
  }

  // Printing the token vector
  for (int i = 0; i < tokens.size(); i++)
    cout << "---  " << tokens[i] << "  --- \n";
}