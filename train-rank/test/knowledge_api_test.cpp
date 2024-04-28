#include <gtest/gtest.h>

#include <unordered_set>

#include "../src/common_tool.h"
#include "../src/knowledgebase_api.h"
#include "test_common.h"

TEST(KnowledgeApiTest, TestUpdateRankScore) {
  // --gtest_filter=KnowledgeApiTest.TestUpdateRankScore
  initDevelop();
  init_log();
  knowledgebase::updateAlgorithmScoreAndRanked("656d09e5a2e46f241f9a7a5d", 20,
                                               false);
}
TEST(KnowledgeApiTest, TestUpdateAlgorithmRankedScored) {
  // --gtest_filter=KnowledgeApiTest.TestUpdateAlgorithmRankedScored
  initDevelop();
  init_log();
  std::unordered_map<std::string, float> entry_id_to_score;
  entry_id_to_score["656d09e6a2e46f241f9a7b89"] = 0.25;
  entry_id_to_score["656d09e6a2e46f241f9a7b8a"] = 0.5;
  knowledgebase::updateAlgorithmScoreAndRanked(entry_id_to_score);
}

TEST(KnowledgeApiTest, TestRerank) {
  // --gtest_filter=KnowledgeApiTest.TestRerank
  initDevelop();
  init_log();
  knowledgebase::rerank("bert_v2");
}

TEST(KnowledgeApiTest, TestGetImpression) {
  // --gtest_filter=KnowledgeApiTest.TestGetImpression
  initDevelop();
  init_log();
  std::vector<Impression> impression_list;
  int count;
  knowledgebase::getImpression(10, 10, "bert_v2", &impression_list, &count);
  std::cout << impression_list.size() << " " << count << std::endl;
}

TEST(KnowledgeApiTest, TestGetAllImpression) {
  // --gtest_filter=KnowledgeApiTest.TestGetAllImpression
  initDevelop();
  init_log();
  std::vector<Impression> impression_list;
  int count;
  knowledgebase::getAllImpression("bert_v2", &impression_list, &count);
  std::unordered_set<std::string> impression_id_set;

  for (auto current_impression : impression_list) {
    impression_id_set.insert(current_impression.id);
  }
  std::cout << impression_id_set.size() << "  ****  " << count << std::endl;
}

TEST(KnowledgeApiTest, TestGetImpressionById) {
  // --gtest_filter=KnowledgeApiTest.TestGetImpressionById
  initDevelop();
  init_log();
  std::string impression_id = "656d09e5a2e46f241f9a79fa";
  std::optional<Impression> current_impression =
      knowledgebase::GetImpressionById(impression_id);
  if (current_impression != std::nullopt) {
    std::cout << current_impression.value().entry_id << "  "
              << current_impression.value().source << std::endl;
  }
}

TEST(KnowledgeApiTest, TestGetAlgorithmById) {
  // --gtest_filter=KnowledgeApiTest.TestGetAlgorithmById
  initDevelop();
  init_log();
  std::string algorithm_id = "659f62bcd0158a3187db94c1";
  std::optional<Algorithm> current_algorithm =
      knowledgebase::GetAlgorithmById(algorithm_id);
  if (current_algorithm != std::nullopt) {
    std::cout << current_algorithm.value().entry << "  "
              << current_algorithm.value().source << std::endl;
  }
}

TEST(KnowledgeApiTest, TestGetEntryById) {
  // --gtest_filter=KnowledgeApiTest.TestGetEntryById
  initDevelop();
  init_log();
  std::string entry_id = "65f3f335486181b2934406bc";
  std::optional<Entry> current_entry = knowledgebase::GetEntryById(entry_id);
  if (current_entry != std::nullopt) {
    std::cout << current_entry.value().url << "  "
              << current_entry.value().title << std::endl;
  }
}

TEST(KnowledgeApiTest, TestUpdateLastRankTime) {
  // --gtest_filter=KnowledgeApiTest.TestUpdateLastRankTime
  initDevelop();
  init_log();
  knowledgebase::updateLastRankTime("bert_v2", 1711080225);
}


TEST(KnowledgeApiTest, TestUpdateLastExtractorTime) {
  // --gtest_filter=KnowledgeApiTest.TestUpdateLastExtractorTime
  initDevelop();
  init_log();
  knowledgebase::updateLastExtractorTime("bert_v2", 1711080229);
}

TEST(KnowledgeApiTest, TestGetLastRankTime) {
  // --gtest_filter=KnowledgeApiTest.TestGetLastRankTime
  initDevelop();
  init_log();
  int64_t lask_rank_time = knowledgebase::getLastRankTime("bert_v2");
  std::cout << "last_rank_time " <<lask_rank_time << std::endl;
}

TEST(KnowledgeApiTest, TestGetLastExtractTime) {
  // --gtest_filter=KnowledgeApiTest.TestGetLastExtractTime
  initDevelop();
  init_log();
  int64_t last_extractor_time = knowledgebase::getLastExtractorTime("bert_v2");
  std::cout << "last_extractor_time " <<last_extractor_time << std::endl;
}



TEST(KnowledgeApiTest, TestGetAlgorithmAccordingRanked) {
  // --gtest_filter=KnowledgeApiTest.TestGetAlgorithmAccordingRanked
  initDevelop();
  init_log();
  std::vector<Algorithm> algorithm_list;
  int count;
  knowledgebase::getAlgorithmAccordingRanked(10, 10, "bert_v2", true,
                                             &algorithm_list, &count);
  std::cout << algorithm_list.size() << " " << count << std::endl;
  if (algorithm_list.size() > 0) {
    Algorithm temp = algorithm_list[0];
    std::cout << std::boolalpha << temp.id << " " << temp.entry << " "
              << temp.ranked << " ok" << std::endl;
  }
}

TEST(KnowledgeApiTest, TestGetAllAlgorithmAccordingRanked) {
  // --gtest_filter=KnowledgeApiTest.TestGetAllAlgorithmAccordingRanked
  initDevelop();
  init_log();
  std::vector<Algorithm> algorithm_list;
  int count;
  knowledgebase::getAllAlgorithmAccordingRanked("bert_v2", &algorithm_list,
                                                true, &count);
  std::unordered_set<std::string> algorithm_id_set;

  for (auto current_impression : algorithm_list) {
    algorithm_id_set.insert(current_impression.id);
  }
  std::cout << algorithm_id_set.size() << "  ****  " << count << std::endl;
}