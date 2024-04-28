#include "../src/rssrank.h"

#include <gtest/gtest.h>
#include <stdlib.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "../src/common_tool.h"
#include "../src/xgboost_macro.hpp"
#include "test_common.h"

TEST(BertV2Test, predictRankTest) {
  // --gtest_filter=BertV2Test.predictRankTest
  initDevelop();
  init_log();
  rssrank::rankPredict();
}

TEST(BertV2Test, BertV2TrainBigBatchTest) {
  // --gtest_filter=BertV2Test.BertV2TrainBigBatchTest
  initDevelop();
  init_log();

  rssrank::trainOneBigBatch();
}

TEST(BertV2Test, TestGetNotRankedAlgorithmEntry) {
  // --gtest_filter=BertV2Test.TestGetNotRankedAlgorithmEntry
  initDevelop();
  init_log();
  // std::vector<std::string> algorithm_id_list =
  // rssrank::getNotRankedAlgorithmEntry(); std::cout << "algorithm id size " <<
  // algorithm_id_list.size() << std::endl;
}

TEST(BertV2Test, getAllEntryToPrerankSourceForCurrentSourceKnowledge) {
  // --gtest_filter=BertV2Test.getAllEntryToPrerankSourceForCurrentSourceKnowledge
  initDevelop();
  init_log();
  std::unordered_map<std::string, float> algorithm_entry_to_prerank_score =
      rssrank::getAllEntryToPrerankSourceForCurrentSourceKnowledge();
  for (auto current : algorithm_entry_to_prerank_score) {
    std::cout << current.first << " " << current.second << std::endl;
  }
}

TEST(BertV2Test, getRankModelPath) {
  // --gtest_filter=BertV2Test.getRankModelPath
  initDevelop();
  init_log();
  rssrank::getRankModelPath(rssrank::ModelPathType::PRODUCTION);
}