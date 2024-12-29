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
  entry_id_to_score["656d09e6a2e46f241f9a7b89"] = ScoreWithMetadata(0.25, getTimeStampNow(), ScoreEnum::SCORE_UNKNOWN);
  entry_id_to_score["656d09e6a2e46f241f9a7b8a"] = ScoreWithMetadata(0.5, getTimeStampNow(), ScoreEnum::SCORE_UNKNOWN);
  knowledgebase::updateAlgorithmScoreAndMetadata(entry_id_to_score);
}

TEST(KnowledgeApiTest, updateAlgorithmScoreAndMetadataWithScoreOrder)
{
  // --gtest_filter=KnowledgeApiTest.updateAlgorithmScoreAndMetadataWithScoreOrder
  initDevelop();
  init_log();
  std::unordered_map<std::string, std::string> algorithm_id_to_entry_id =
      rssrank::getNotImpressionedAlgorithmToEntry();
  std::cout << "algorithm_id_to_entry_id size " << algorithm_id_to_entry_id.size() << std::endl;
  int index = 1;
  std::vector<std::string> modified_algorithm_list;
  for (auto item : algorithm_id_to_entry_id)
  {
    std::cout << item.first << " " << item.second << std::endl;
    if (index >= 3)
    {
      break;
    }
    index++;
    modified_algorithm_list.push_back(item.first);
  }
  std::cout << "modified_algorithm_list size " << modified_algorithm_list.size() << std::endl;

  std::unordered_map<std::string, ScoreWithMetadata> algorithm_id_to_score;
  long long current_time = getTimeStampNow();
  int sequence_index = 3;
  for (auto item : modified_algorithm_list)
  {
    std::cout << "algorithm_id updated" << item << std::endl;
    algorithm_id_to_score[item] = ScoreWithMetadata(0.71, current_time, ScoreEnum::SCORE_UNKNOWN);
    algorithm_id_to_score[item].score_rank_sequence = sequence_index;
    std::cout << "999999999999999999999 algorithm_id_to_score[item].score_rank_sequence " << algorithm_id_to_score[item].score_rank_sequence << std::endl;
    sequence_index = sequence_index + 1;
  }
  knowledgebase::updateAlgorithmScoreAndMetadataWithScoreOrder(algorithm_id_to_score);
  int previous_sequence = -1;
  for (auto item : algorithm_id_to_score)
  {
    Algorithm new_get_algorithm = knowledgebase::GetAlgorithmById(item.first).value();
    EXPECT_EQ(new_get_algorithm.ranked, true);
    EXPECT_FLOAT_EQ(new_get_algorithm.score, 0.71);
    std::cout << "new_get_algorithm.score_rank_sequence " << new_get_algorithm.score_rank_sequence << " original sequnce " << item.second.score_rank_sequence << std::endl;
    EXPECT_GT(new_get_algorithm.score_rank_sequence, previous_sequence);
    previous_sequence = new_get_algorithm.score_rank_sequence;
    // EXPECT_EQ(new_get_algorithm.score_rank_sequence, item.second.score_rank_sequence);
  }
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
  knowledgebase::getImpression(10, 10, "r4techbiz", &impression_list, &count);
  for (auto current_impression : impression_list)
  {
    std::cout << current_impression.id << " " << current_impression.source << " " << current_impression.integer_id << std::endl;
  }
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
    std::cout << current.id << " " << current.impression << " " << current.integer_id << std::endl;
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
    std::cout << current.url << " " << current.title << " " << current.published_at << " " << current.integer_id << std::endl;
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
#include "../src/userembedding_calculation.h"
TEST(RssRankTest, TestCalculateEmbeddingMultipleReal)
{
  // --gtest_filter=RssRankTest.TestCalculateEmbeddingMultipleReal
  initDevelop();
  init_log();
  knowledgebase::EntryCache::getInstance().init();
  std::vector<Impression> result = rssrank::getImpressionForShortTermAndLongTermUserEmbeddingRank();
  std::vector<double> embedding = calcluateUserShortTermEmbedding(result, true);
  double total = 0;
  for (auto current : embedding)
  {
    std::cout << current << " ";
    total += current * current;
  }
  std::cout << "total " << total << std::endl;
  std::vector<double> embedding2 = calcluateUserLongTermEmbedding(result);
  double total2 = 0;
  for (auto current : embedding2)
  {
    std::cout << current << " ";
    total2 += current * current;
  }
  std::cout << "total2 " << total2 << std::endl;
}

TEST(RssRankTest, TestFaissSearch)
{
  // --gtest_filter=RssRankTest.TestFaissSearch
  initDevelop();
  init_log();
  knowledgebase::EntryCache::getInstance().init();
  std::vector<Impression> impression_list = rssrank::getImpressionForShortTermAndLongTermUserEmbeddingRank();
  FAISSArticleSearch search(impression_list);
  std::unordered_map<std::string, std::string> algorithm_id_to_entry_id = rssrank::getNotImpressionedAlgorithmToEntry();
  for (const auto &current_item : algorithm_id_to_entry_id)
  {
    std::optional<Entry> temp_entry = knowledgebase::EntryCache::getInstance().getEntryById(current_item.second);
    if (temp_entry == std::nullopt)
    {
      LOG(ERROR) << "entry [" << current_item.second << "] not exist, algorithm [" << current_item.first << "]" << std::endl;
      continue;
    }
    if (!temp_entry.value().extract)
    {
      LOG(ERROR) << "entry [" << current_item.second << "] not extract, algorithm [" << current_item.first << "]" << std::endl;
      continue;
    }
    std::optional<Algorithm> current_algorithm = knowledgebase::GetAlgorithmById(current_item.first);
    if (current_algorithm == std::nullopt)
    {
      LOG(ERROR) << "Algorithm item not found, id = " << current_item.first << std::endl;
      continue;
    }
    if (current_algorithm.value().embedding == std::nullopt)
    {
      LOG(ERROR) << "Algorithm item no embbeding found, id = " << current_item.first << std::endl;
      continue;
    }
    std::pair<int, double> result = search.findMostSimilarArticle(current_algorithm.value().embedding.value());
    Entry similared_entry = knowledgebase::EntryCache::getInstance().getEntryById(impression_list[result.first].entry_id).value();
    std::cout << "current_item [" << temp_entry.value().title << "] most similar [" << similared_entry.title << "]" << std::endl;
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
  std::vector<double> result = calcluateUserShortTermEmbedding(impressions, false);
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
  knowledgebase::updateRecallUserEmbedding(source, user_embedding, getTimeStampNow());
  std::vector<double> result = knowledgebase::getRecallUserEmbedding(source);
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
  articles.clear();
  articles.shrink_to_fit(); // Clear the articles vector
  // Create FAISSArticleSearch object

  // Check if FAISS index is created and populated
  std::vector<float> query = {1.0f, 0.0f, 0.0f};
  auto [index, distance] = search.findMostSimilarArticle(query);
  std::cout << "Most similar article: " << index << " Distance: " << distance << std::endl;
  assert(index == 0);                // The most similar article should be the first one
  assert(std::abs(distance) < 1e-6); // Distance should be 0

  query = {0.5f, 0.5f, 0.0f};
  auto [index2, distance2] = search.findMostSimilarArticle(query);
  std::cout << "Most similar article: " << index2 << " Distance: " << distance2 << std::endl;
  assert(index2 == 3); // The most similar article should be the last one

  query = {0.5f, 0.5f, 0.5f};
  auto [index3, distance3] = search.findMostSimilarArticle(query);
  std::cout << "Most similar article: " << index3 << " Distance: " << distance3 << std::endl;
  assert(index3 == 3); // The most similar article should be the last one
}

TEST(RssRankTest, RankShortTermAndLongTermUserEmbedding)
{
  // --gtest_filter=RssRankTest.RankShortTermAndLongTermUserEmbedding
  initDevelop();
  init_log();
  rssrank::rankShortTermAndLongTermUserEmbedding();
}

TEST(RssRankTest, rankByTimeForColdStart)
{
  // --gtest_filter=RssRankTest.rankByTimeForColdStart
  initDevelop();
  init_log();
  vector<Impression> clicked_impressions = rssrank::getImpressionForShortTermAndLongTermUserEmbeddingRank();
  rssrank::rankByTimeForColdStart(getTimeStampNow(), clicked_impressions);
}

TEST(KnowledgeApiTest, postRecommendTraceUserEmbedding)
{
  // --gtest_filter=KnowledgeApiTest.postRecommendTraceUserEmbedding
  initDevelop();
  init_log();
  RecommendTraceUserEmbedding recommend_trace_user_embedding;
  recommend_trace_user_embedding.source = "testsource";
  recommend_trace_user_embedding.user_embedding = {1.0, 2.0, 3.0};
  recommend_trace_user_embedding.created_rank_time = 1711080226;
  recommend_trace_user_embedding.calculateUniqueId();
  recommend_trace_user_embedding.setImpressionIdUsedToCalculateEmbedding({1, 2, 3});
  knowledgebase::postRecommendTraceUserEmbedding(recommend_trace_user_embedding);
}

TEST(KnowledgeApiTest, findRecommendTraceUserEmbeddingByUniqueId)
{
  // --gtest_filter=KnowledgeApiTest.findRecommendTraceUserEmbeddingByUniqueId
  initDevelop();
  init_log();
  std::string unique_id = "650637621e5f7523b818def76a5d7f64d6a10f41ac751a7cb085fd6330f90874";
  std::optional<RecommendTraceUserEmbedding> recommend_trace_user_embedding = knowledgebase::findRecommendTraceUserEmbeddingByUniqueId(unique_id);
  if (recommend_trace_user_embedding != std::nullopt)
  {
    RecommendTraceUserEmbedding current = recommend_trace_user_embedding.value();
    std::cout << current << " " << std::endl;
  }
}

TEST(KnowledgeApiTest, postRecommendTraceInfo)
{
  // --gtest_filter=KnowledgeApiTest.postRecommendTraceInfo
  initDevelop();
  init_log();
  RecommendTraceInfo recommend_trace_info;
  recommend_trace_info.source = "testsource";
  recommend_trace_info.rank_time = 1711080226;
  recommend_trace_info.previous_rank_time = 1711080225;
  recommend_trace_info.score_enum = "score_enum";
  recommend_trace_info.not_impressioned_algorithm_id = "1-3;6;8-9";
  recommend_trace_info.added_not_impressioned_algorithm_id = "8-9";
  recommend_trace_info.impressioned_clicked_id = "1-20";
  recommend_trace_info.added_impressioned_clicked_id = "20";
  recommend_trace_info.long_term_user_embedding_id = "111111";
  recommend_trace_info.short_term_user_embedding_id = "222222";
  recommend_trace_info.recall_user_embedding_id = "333333";
  recommend_trace_info.top_ranked_algorithm_id = {1, 2, 3};
  recommend_trace_info.top_ranked_algorithm_score = {0.1, 0.2, 0.3};
  knowledgebase::postRecommendTraceInfo(recommend_trace_info);
}

TEST(KnowledgeApiTest, findRecommendTraceInfoByRankTimeAndSource)
{
  // --gtest_filter=KnowledgeApiTest.findRecommendTraceInfoByRankTimeAndSource
  initDevelop();
  init_log();
  std::string source = "testsource";
  int64_t rank_time = 1711080226;
  std::optional<RecommendTraceInfo> recommend_trace_info = knowledgebase::findRecommendTraceInfoByRankTimeAndSource(source, rank_time);
  if (recommend_trace_info != std::nullopt)
  {
    RecommendTraceInfo current = recommend_trace_info.value();
    std::cout << current << " " << std::endl;
  }
}

TEST(KnowledgeApiTest, findAllRecomendTraceInfoRankTimesBySource)
{
  // --gtest_filter=KnowledgeApiTest.findAllRecomendTraceInfoRankTimesBySource
  initDevelop();
  init_log();
  std::string source = "testsource";
  std::vector<int> rank_times = knowledgebase::findAllRecomendTraceInfoRankTimesBySource(source);
  for (auto current : rank_times)
  {
    std::cout << current << " ";
  }
  std::cout << std::endl;
}

TEST(KnowledgeApiTest, getKnowledgeCofnig)
{
  // --gtest_filter=KnowledgeApiTest.getKnowledgeCofnig
  initDevelop();
  init_log();

  std::string source = "r4techbiz";

  std::optional<string> value = knowledgebase::getKnowledgeCofnig(source, knowledgebase::LAST_RANK_TIME);
  std::cout << "LAST_RANK_TIME " << value.value() << std::endl;
  value = knowledgebase::getKnowledgeCofnig("recommend_parameter", "long_weight");
  std::cout << "long_weight " << value.value() << " " << value.value().length() << std::endl;
}