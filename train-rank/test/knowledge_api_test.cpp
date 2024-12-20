#include <gtest/gtest.h>

#include <unordered_set>

#include "../src/common_tool.h"
#include "../src/knowledgebase_api.h"
#include "../src/entity/reco_metadata.h"
#include "../src/rssrank.h"
#include "../src/faiss_article_search.h"
#include "test_common.h"

TEST(KnowledgeApiTest, TestUpdateRankScore)
{
  // --gtest_filter=KnowledgeApiTest.TestUpdateRankScore
  initDevelop();
  init_log();
  knowledgebase::updateAlgorithmScoreAndRanked("656d09e5a2e46f241f9a7a5d", 20,
                                               false);
}
TEST(KnowledgeApiTest, TestUpdateAlgorithmRankedScored)
{
  // --gtest_filter=KnowledgeApiTest.TestUpdateAlgorithmRankedScored
  initDevelop();
  init_log();
  std::unordered_map<std::string, ScoreWithMetadata> entry_id_to_score;
  entry_id_to_score["656d09e6a2e46f241f9a7b89"] = ScoreWithMetadata(0.25);
  entry_id_to_score["656d09e6a2e46f241f9a7b8a"] = ScoreWithMetadata(0.5);
  knowledgebase::updateAlgorithmScoreAndMetadata(entry_id_to_score);
}

TEST(KnowledgeApiTest, TestRerank)
{
  // --gtest_filter=KnowledgeApiTest.TestRerank
  initDevelop();
  init_log();
  knowledgebase::rerank("bert_v2");
}

TEST(KnowledgeApiTest, TestGetImpression)
{
  // --gtest_filter=KnowledgeApiTest.TestGetImpression
  initDevelop();
  init_log();
  std::vector<Impression> impression_list;
  int count;
  knowledgebase::getImpression(10, 10, "bert_v2", &impression_list, &count);
  std::cout << impression_list.size() << " " << count << std::endl;
}

TEST(KnowledgeApiTest, TestGetAllImpression)
{
  // --gtest_filter=KnowledgeApiTest.TestGetAllImpression
  initDevelop();
  init_log();
  std::vector<Impression> impression_list;
  int count;
  knowledgebase::getAllImpression("r4techbiz", &impression_list, &count);
  std::unordered_set<std::string> impression_id_set;

  for (auto current_impression : impression_list)
  {
    impression_id_set.insert(current_impression.id);
  }
  std::cout << impression_id_set.size() << "  ****  " << count << std::endl;
}

TEST(KnowledgeApiTest, TestGetImpressionById)
{
  // --gtest_filter=KnowledgeApiTest.TestGetImpressionById
  initDevelop();
  init_log();
  std::string impression_id = "656d09e5a2e46f241f9a79fa";
  std::optional<Impression> current_impression =
      knowledgebase::GetImpressionById(impression_id);
  if (current_impression != std::nullopt)
  {
    std::cout << current_impression.value().entry_id << "  "
              << current_impression.value().source << std::endl;
  }
}

TEST(KnowledgeApiTest, TestGetAlgorithmById)
{
  // --gtest_filter=KnowledgeApiTest.TestGetAlgorithmById
  initDevelop();
  init_log();
  std::string algorithm_id = "659f62bcd0158a3187db94c1";
  std::optional<Algorithm> current_algorithm =
      knowledgebase::GetAlgorithmById(algorithm_id);
  if (current_algorithm != std::nullopt)
  {
    std::cout << current_algorithm.value().entry << "  "
              << current_algorithm.value().source << std::endl;
  }
}

TEST(KnowledgeApiTest, TestGetEntryById)
{
  // --gtest_filter=KnowledgeApiTest.TestGetEntryById
  initDevelop();
  init_log();
  std::string entry_id = "65f3f335486181b2934406bc";
  std::optional<Entry> current_entry = knowledgebase::GetEntryById(entry_id);
  if (current_entry != std::nullopt)
  {
    std::cout << current_entry.value().url << "  "
              << current_entry.value().title << std::endl;
  }
}

TEST(KnowledgeApiTest, TestUpdateLastRankTime)
{
  // --gtest_filter=KnowledgeApiTest.TestUpdateLastRankTime
  initDevelop();
  init_log();
  knowledgebase::updateLastRankTime("bert_v2", 1711080225);
}

TEST(KnowledgeApiTest, TestUpdateLastExtractorTime)
{
  // --gtest_filter=KnowledgeApiTest.TestUpdateLastExtractorTime
  initDevelop();
  init_log();
  knowledgebase::updateLastExtractorTime("r4world", 1711080229);
}

TEST(KnowledgeApiTest, TestGetLastRankTime)
{
  // --gtest_filter=KnowledgeApiTest.TestGetLastRankTime
  initDevelop();
  init_log();
  int64_t lask_rank_time = knowledgebase::getLastRankTime("bert_v2");
  std::cout << "last_rank_time " << lask_rank_time << std::endl;
}

TEST(KnowledgeApiTest, TestGetLastExtractTime)
{
  // --gtest_filter=KnowledgeApiTest.TestGetLastExtractTime
  initDevelop();
  init_log();
  int64_t last_extractor_time = knowledgebase::getLastExtractorTime("bert_v2");
  std::cout << "last_extractor_time " << last_extractor_time << std::endl;
}

TEST(KnowledgeApiTest, TestEntryCacheInit)
{
  // --gtest_filter=KnowledgeApiTest.TestEntryCacheInit
  initDevelop();
  init_log();
  knowledgebase::EntryCache::getInstance().init();
}

TEST(KnowledgeApiTest, TestGetAlgorithmAccordingRanked)
{
  // --gtest_filter=KnowledgeApiTest.TestGetAlgorithmAccordingRanked
  initDevelop();
  init_log();
  std::vector<Algorithm> algorithm_list;
  int count;
  knowledgebase::getAlgorithmAccordingRanked(0, 10, "worldnews", false,
                                             &algorithm_list, &count);
  std::cout << algorithm_list.size() << " " << count << std::endl;
  if (algorithm_list.size() > 0)
  {
    Algorithm temp = algorithm_list[0];
    std::cout << std::boolalpha << temp.id << " " << temp.entry << " "
              << temp.ranked << " ok" << std::endl;
  }
}

TEST(KnowledgeApiTest, TestGetAllAlgorithmAccordingRanked)
{
  // --gtest_filter=KnowledgeApiTest.TestGetAllAlgorithmAccordingRanked
  initDevelop();
  init_log();
  std::vector<Algorithm> algorithm_list;
  int count;
  knowledgebase::getAllAlgorithmAccordingRanked("worldnews", &algorithm_list,
                                                true, &count);
  std::unordered_set<std::string> algorithm_id_set;

  for (auto current_impression : algorithm_list)
  {
    algorithm_id_set.insert(current_impression.id);
  }
  std::cout << algorithm_id_set.size() << "  ****  " << count << std::endl;
}

TEST(KnowledgeApiTest, TestGetAlgorithmAccordingImpression)
{
  // --gtest_filter=KnowledgeApiTest.TestGetAlgorithmAccordingImpression
  initDevelop();
  init_log();
  std::vector<Algorithm> algorithm_list;
  int count;
  std::string source = "r4techbiz";
  knowledgebase::getAlgorithmAccordingImpression(10, 0, source, 1,
                                                 &algorithm_list, &count);
  for (auto current : algorithm_list)
  {
    std::cout << current.id << " " << current.impression << std::endl;
  }
  for (int i = 0; i < 10; i++)
  {
    knowledgebase::getAlgorithmAccordingImpression(10, 0, source, 1,
                                                   &algorithm_list, &count);
  }
}

TEST(KnowledgeApiTest, getNotImpressionedAlgorithmToEntry)
{
  // --gtest_filter=KnowledgeApiTest.getNotImpressionedAlgorithmToEntry
  initDevelop();
  init_log();
  std::unordered_map<std::string, std::string> not_impressioned_algorithm_to_entry =
      rssrank::getNotImpressionedAlgorithmToEntry();
  std::cout << not_impressioned_algorithm_to_entry.size() << std::endl;
}

TEST(KnowledgeApiTest, TestGetEntries)
{
  // --gtest_filter=KnowledgeApiTest.TestGetEntries
  initDevelop();
  init_log();
  std::vector<Entry> entry_list;
  int count;
  std::string source = "r4techbiz";
  knowledgebase::getEntries(10, 2, source, &entry_list, &count);
  std::cout << entry_list.size() << " " << count << std::endl;
  for (auto current : entry_list)
  {
    std::cout << current.url << " " << current.title << " " << current.published_at << std::endl;
  }
}

TEST(KnowledgeApiTest, TestGetNotRankedAlgorithmToEntry)
{
  // --gtest_filter=KnowledgeApiTest.TestGetNotRankedAlgorithmToEntry
  initDevelop();
  init_log();
  std::unordered_map<std::string, std::string> not_ranked_algorithm_to_entry =
      rssrank::getNotRankedAlgorithmToEntry();
  for (auto current : not_ranked_algorithm_to_entry)
  {
    std::cout << current.first << " " << current.second << std::endl;
  }
}

TEST(RssRankTest, getImpressionForShortTermAndLongTermUserEmbeddingRank)
{
  // --gtest_filter=RssRankTest.getImpressionForShortTermAndLongTermUserEmbeddingRank
  initDevelop();
  init_log();
  knowledgebase::EntryCache::getInstance().init();
  std::vector<Impression> result = rssrank::getImpressionForShortTermAndLongTermUserEmbeddingRank();
  for (auto current : result)
  {
    std::cout << "current_last_opened " << current.entry_last_opened << std::endl;
  }
}

TEST(RssRankTest, TestCalculateEmbeddingMultipleReal)
{
  // --gtest_filter=RssRankTest.TestCalculateEmbeddingMultipleReal
  initDevelop();
  init_log();
  knowledgebase::EntryCache::getInstance().init();
  std::vector<Impression> result = rssrank::getImpressionForShortTermAndLongTermUserEmbeddingRank();
  std::vector<double> embedding = rssrank::calcluateUserShortTermEmbedding(result, true);
  for (auto current : embedding)
  {
    std::cout << current << " ";
  }
}

TEST(RssRankTest, TestCalculateEmbeddingMultiple)
{
  // --gtest_filter=RssRankTest.TestCalculateEmbeddingMultiple
  initDevelop();
  init_log();

  Impression impression1, impression2;
  impression1.embedding = std::vector<double>{1.0, 2.0, 3.0};
  impression2.embedding = std::vector<double>{4.0, 5.0, 6.0};
  std::vector<Impression> impressions = {impression1, impression2};
  std::vector<double> result = rssrank::calcluateUserShortTermEmbedding(impressions, false);
  ASSERT_EQ(result.size(), 3);
  EXPECT_FLOAT_EQ(result[0], 5.0);
  EXPECT_FLOAT_EQ(result[1], 7.0);
  EXPECT_FLOAT_EQ(result[2], 9.0);
}

TEST(RssRankTest, TestParseEmbedding)
{
  //--gtest_filter=RssRankTest.TestParseEmbedding
  std::string input = "1.2;1.3;1.4;1777233344"; // Example input
  size_t embedding_dimension = 3;               // Example embedding dimension

  std::vector<double> embedding = knowledgebase::parse_embedding(input, embedding_dimension);

  if (embedding.empty())
  {
    std::cout << "Invalid input or format." << std::endl;
  }
  else
  {
    std::cout << "Parsed embedding: ";
    for (double val : embedding)
    {
      std::cout << val << " ";
    }
    std::cout << std::endl;
  }
}

TEST(RssRankTest, TestInitUserEmbedding)
{
  // --gtest_filter=RssRankTest.TestInitUserEmbedding
  size_t embedding_dimension = 384; // Example embedding dimension

  std::vector<double> user_embedding = knowledgebase::init_user_embedding(embedding_dimension);

  std::cout << "User embedding: ";
  for (double val : user_embedding)
  {
    std::cout << val << " ";
  }
  std::cout << std::endl;
}

TEST(RssRankTest, TestSetUserEmbedding)
{
  // --gtest_filter=RssRankTest.TestSetUserEmbedding
  initDevelop();
  init_log();
  size_t embedding_dimension = 384; // Example embedding dimension

  std::vector<double> user_embedding = knowledgebase::init_user_embedding(embedding_dimension);
  std::string source = "r4techbiz";
  knowledgebase::updateLongTermUserEmbedding(source, user_embedding);
  std::vector<double> result = knowledgebase::getLongTermUserEmbedding(source);
  bool equal = true;
  for (int i = 0; i < result.size(); i++)
  {
    if (are_equal_double(result[i], user_embedding[i]) == false)
    {
      equal = false;
      break;
    }
  }
  std::cout << "eaul " << equal << std::endl;
}

TEST(RssRankTest, getTimeCoefficientForUnixTimestamp)
{
  // --gtest_filter=RssRankTest.getTimeCoefficientForUnixTimestamp
  initDevelop();
  init_log();
  int64_t timestamp = 1711080225; // Example timestamp
  double time_coefficient = rssrank::getTimeCoefficientForUnixTimestamp(timestamp, timestamp);
  std::cout << "Time coefficient: " << time_coefficient << std::endl;
  double time_coefficient2 = rssrank::getTimeCoefficientForUnixTimestamp(timestamp, timestamp + 24 * 60 * 60);
  std::cout << "Time coefficient: " << time_coefficient2 << std::endl;
  double time_coefficient3 = rssrank::getTimeCoefficientForUnixTimestamp(timestamp, timestamp + 24 * 60 * 60 * 7);
  std::cout << "Time coefficient: " << time_coefficient3 << std::endl;
  double time_coefficient4 = rssrank::getTimeCoefficientForUnixTimestamp(timestamp, timestamp + 24 * 60 * 60 * 30);
  std::cout << "Time coefficient: " << time_coefficient4 << std::endl;
  double time_coefficient5 = rssrank::getTimeCoefficientForUnixTimestamp(timestamp, timestamp + 24 * 60 * 60 * 365);
  std::cout << "Time coefficient: " << time_coefficient5 << std::endl;
}

TEST(RssRankTest, rankShortTermAndLongTermUserEmbedding)
{
  // --gtest_filter=RssRankTest.rankShortTermAndLongTermUserEmbedding
  initDevelop();
  init_log();
  rssrank::rankShortTermAndLongTermUserEmbedding();
}

TEST(FaissSearchTest, findMostSimilar)
{
  // --gtest_filter=FaissSearchTest.findMostSimilar

  std::vector<std::vector<float>> articles = {
      {1.0f, 0.0f, 0.0f},
      {0.0f, 1.0f, 0.0f},
      {0.0f, 0.0f, 1.0f},
      {0.5f, 0.5f, 0.0f}};
  FAISSArticleSearch search(articles);
  // Create FAISSArticleSearch object

  // Check if FAISS index is created and populated
  std::vector<float> query = {1.0f, 0.0f, 0.0f};
  auto [index, distance] = search.findMostSimilarArticle(query);
  assert(index == 0);                // The most similar article should be the first one
  assert(std::abs(distance) < 1e-6); // Distance should be 0
}