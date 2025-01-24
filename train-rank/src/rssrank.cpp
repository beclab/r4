#include "faiss_article_search.h"
#include "rssrank.h"

#include <xgboost/c_api.h>

#include <cppitertools/itertools.hpp>
#include <filesystem>
namespace fs = std::filesystem;

#include <cmath>
#include <random>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/date_time.hpp>
// #include <eigen3/Eigen/Dense>
// using namespace Eigen;

#include "common_tool.h"
#include "data_process.h"
#include "easylogging++.h"
#include "knowledgebase_api.h"
#include "xgboost_macro.hpp"
#include "userembedding_calculation.h"

// #include "entity/reco_metadata.h"

#include "lr/feature_extractor.h"
#include "lr/logistic_regression.h"
#include "lr/model_serializer.h"

using lr::EmbeddingDistanceExtractor;
using lr::FeatureExtractor;
using std::vector;

DEFINE_string(model_path_root, envOrBlank("MODEL_PATH_ROOT"), "Model path root");
DEFINE_string(recommend_source_name, envOrBlank("TERMINUS_RECOMMEND_SOURCE_NAME"), "Terminus recommend source name");
DEFINE_bool(upload_score, true, "Whether upload score to knowledge");
DEFINE_bool(verbose, true, "Whether output all the details");

// Get all the databases from a given client.
namespace rssrank
{

  namespace
  {

    vector<Impression> getImpressionsForLR()
    {
      const int batch_size = 1000;
      int offset = 0;
      int count;

      vector<Impression> impressions;
      while (true)
      {
        LOG(DEBUG) << "offset " << offset << " limit " << batch_size << " count "
                   << count;
        std::vector<Impression> temp_impressions;
        knowledgebase::getImpression(batch_size, offset, FLAGS_recommend_source_name,
                                     &temp_impressions, &count);
        // impression_list.insert(impression_list.end(),temp_impression.begin(),temp_impression.end());
        for (const auto &current_impression : temp_impressions)
        {
          if (current_impression.embedding == std::nullopt)
          {
            LOG(ERROR) << "current_impression " << current_impression.id
                       << " embedding not exist" << std::endl;
            continue;
          }
          size_t current_embedding_dimension =
              current_impression.embedding.value().size();
          if (current_embedding_dimension != getCurrentEmbeddingDimension())
          {
            LOG(ERROR) << "current_impression " << current_impression.id
                       << " embedding dimension size "
                       << current_embedding_dimension << " not equal to "
                       << getCurrentEmbeddingDimension() << std::endl;
            continue;
          }

          impressions.push_back(current_impression);
        }
        offset = offset + batch_size;
        if (offset >= count)
        {
          break;
        }
      }
      return impressions;
    }

    ScoreWithMetadata buildScoreWithMeta(double score, const std::string &entry_id, long long score_rank_time, ScoreEnum score_enum)
    {
      ScoreWithMetadata result;
      result.score = score;
      result.rankExecuted = true;
      result.score_rank_time = score_rank_time;
      result.score_rank_method = scoreEnumToString[score_enum];
      RecoReasonArticle article;
      article.id = entry_id;
      auto entry = knowledgebase::EntryCache::getInstance().getEntryById(entry_id);
      if (entry.has_value())
      {
        article.title = entry.value().title;
        article.url = entry.value().url;
      }
      result.reason.articles.push_back(std::move(article));

      return result;
    }

    std::unique_ptr<lr::LogisticRegression> loadDefaultModel()
    {
      return std::unique_ptr<lr::LogisticRegression>(
          new lr::LogisticRegression(1, vector<double>{1.0, 0.0}));
    }

    double getTimeCoefficient(boost::posix_time::ptime timestamp)
    {
      auto now = boost::posix_time::second_clock::local_time();
      auto diff = (now - timestamp).total_seconds();

      if (diff <= 0)
      {
        return 1.0;
      }

      double result = 1.8 / (1.0 + std::exp(-86400.0 / diff)) - 0.8; // when diff->0,result->1.0; when diff->+infinite,result->0.1; when diff=86400,result=0.55

      return result;
    }

  } // namespace

  double getTimeCoefficientForUnixTimestamp(long long target_timestamp, long long current_timestamp)
  {

    auto now = boost::posix_time::second_clock::local_time();
    auto diff = current_timestamp - target_timestamp;

    if (diff <= 0)
    {
      return 1.0;
    }

    double result = 1.8 / (1.0 + std::exp(-86400.0 / diff)) - 0.8; // when diff->0,result->1.0; when diff->+infinite,result->0.1; when diff=86400,result=0.55

    return result;
  }

  vector<Impression> getImpressionForShortTermAndLongTermUserEmbeddingRank()
  {
    // this method just need positive sampllle
    vector<Impression> lr_impression = getImpressionsForLR();
    knowledgebase::EntryCache cache = knowledgebase::EntryCache::getInstance();
    long long min_last_opened = cache.getMinLastOpened();
    long long max_last_opened = cache.getMaxLastOpened();
    vector<Impression> positive;
    for (auto &impression : lr_impression)
    {
      if (!impression.clicked)
      {
        continue;
      }
      auto entry = cache.getEntryById(impression.entry_id);
      if (!entry.has_value())
      {
        LOG(ERROR) << "Entry " << impression.entry_id << " not found" << std::endl;
        impression.entry_last_opened = 0;
        continue;
      }
      impression.entry_last_opened = entry.value().last_opened;
      positive.push_back(impression);
    }
    std::sort(positive.begin(), positive.end(), [](const Impression &a, const Impression &b)
              { return a.entry_last_opened > b.entry_last_opened; });
    return positive;
  }

  std::string getRankModelPath(ModelPathType model_path_type)
  {
    const char *source_name = std::getenv(TERMINUS_RECOMMEND_SOURCE_NAME);
    if (source_name == nullptr)
    {
      LOG(ERROR) << TERMINUS_RECOMMEND_SOURCE_NAME << " NOT EXIST" << std::endl;
      exit(-1);
    }
    LOG(INFO) << TERMINUS_RECOMMEND_SOURCE_NAME << " " << source_name
              << std::endl;

    const char *embedding_name = std::getenv(TERMINUS_RECOMMEND_EMBEDDING_NAME);
    if (embedding_name == nullptr)
    {
      LOG(ERROR) << TERMINUS_RECOMMEND_EMBEDDING_NAME << " NOT EXIST"
                 << std::endl;
      exit(-1);
    }
    LOG(INFO) << TERMINUS_RECOMMEND_EMBEDDING_NAME << " " << embedding_name
              << std::endl;

    std::string full_model_directory = std::string(RECOMMEND_MODEL_ROOT) + "/" +
                                       std::string(source_name) + "/" +
                                       std::string(embedding_name);
    if (!fs::exists(full_model_directory))
    {
      if (!fs::create_directories(full_model_directory))
      {
        LOG(ERROR) << "create full model directory fail" << full_model_directory
                   << std::endl;
        exit(-1);
      }
    }
    if (model_path_type == ModelPathType::PRODUCTION)
    {
      return full_model_directory + "/" + "production.json";
    }
    else
    {
      return full_model_directory + "/" + "training.json";
    }
  }

  float getSpecificImpressionScoreForShortTermUserEmbedding(const Impression &current_impression)
  {
    float total_score = 0;
    if (current_impression.clicked)
    {
      total_score = total_score + rssrank::short_term_user_embedding_clicked_weight;
      if (current_impression.read_finish)
      {
        total_score = total_score + rssrank::short_term_user_embedding_read_finish_weight;
      }
      if (current_impression.stared)
      {
        total_score = total_score + rssrank::short_term_user_embedding_stared_weight;
      }
      total_score = total_score + rssrank::short_term_user_embedding_time_weight * getTimeCoefficientForUnixTimestamp(current_impression.entry_last_opened, getTimeStampNow());
    }
    return total_score;
  }
  float getSpecificImpressionScore(const Impression &current_impression)
  {
    // to do
    float total_score = 0;
    if (current_impression.clicked)
    {
      total_score = total_score + rssrank::clicked_weight;
      if (current_impression.read_finish)
      {
        total_score = total_score + rssrank::read_finish_weight;
      }
      if (current_impression.stared)
      {
        total_score = total_score + rssrank::star_weight;
      }
    }
    return total_score;
  }

  std::unordered_map<std::string, std::string> getNotImpressionedAlgorithmToEntry()
  {
    const int batch_size = 100;
    int offset = 0;
    const char *source_name = std::getenv(TERMINUS_RECOMMEND_SOURCE_NAME);
    std::unordered_map<std::string, std::string> algorithm_id_to_entry_id;
    // Load unranked items
    while (true)
    {
      std::vector<Algorithm> temp_algorithm;
      int count = 0;
      knowledgebase::getAlgorithmAccordingImpression(batch_size, offset, source_name,
                                                     0, &temp_algorithm, &count);
      LOG(DEBUG) << "offset " << offset << " limit " << batch_size << " count "
                 << count;
      for (Algorithm current : temp_algorithm)
      {
        if (current.embedding == std::nullopt)
        {
          LOG(ERROR) << "current algorithm " << current.id << " have no embedding"
                     << std::endl;
          continue;
        }
        if (current.embedding.value().size() != getCurrentEmbeddingDimension())
        {
          LOG(ERROR) << "current algorithm " << current.id << " embedding size "
                     << current.embedding.value().size()
                     << " is not equal embedding dimension " << std::endl;
          continue;
        }
        algorithm_id_to_entry_id.emplace(
            std::make_pair(current.id, current.entry));
      }
      offset = offset + batch_size;
      if (offset >= count)
      {
        break;
      }
    }
    offset = 0;
    // Load ranked items
    /**
    while (true)
    {
      std::vector<Algorithm> temp_algorithm;
      int count = 0;
      knowledgebase::getAlgorithmAccordingRanked(batch_size, offset, source_name,
                                                 true, &temp_algorithm, &count);
      LOG(DEBUG) << "offset " << offset << " limit " << batch_size << " count "
                 << count;
      for (Algorithm current : temp_algorithm)
      {
        if (current.embedding == std::nullopt)
        {
          LOG(ERROR) << "current algorithm " << current.id << " have no embedding"
                     << std::endl;
          continue;
        }
        if (current.embedding.value().size() != getCurrentEmbeddingDimension())
        {
          LOG(ERROR) << "current algorithm " << current.id << " embedding size "
                     << current.embedding.value().size()
                     << " is not equal embedding dimension " << std::endl;
          continue;
        }
        if (current.impression > 0)
        {
          continue;
        }
        algorithm_id_to_entry_id.emplace(
            std::make_pair(current.id, current.entry));
      }
      offset = offset + batch_size;
      if (offset >= count)
      {
        break;
      }
    }
    */
    return algorithm_id_to_entry_id;
  }

  std::unordered_map<std::string, std::string> getNotRankedAlgorithmToEntry()
  {
    const int batch_size = 100;
    int offset = 0;
    const char *source_name = std::getenv("TERMINUS_RECOMMEND_SOURCE_NAME");
    std::unordered_map<std::string, std::string> algorithm_id_to_entry_id;
    // Load unranked items
    while (true)
    {
      std::vector<Algorithm> temp_algorithm;
      int count = 0;
      knowledgebase::getAlgorithmAccordingRanked(batch_size, offset, source_name,
                                                 false, &temp_algorithm, &count);
      LOG(DEBUG) << "offset " << offset << " limit " << batch_size << " count "
                 << count;
      for (Algorithm current : temp_algorithm)
      {
        if (current.embedding == std::nullopt)
        {
          LOG(ERROR) << "current algorithm " << current.id << " have no embedding"
                     << std::endl;
          continue;
        }
        if (current.embedding.value().size() != getCurrentEmbeddingDimension())
        {
          LOG(ERROR) << "current algorithm " << current.id << " embedding size "
                     << current.embedding.value().size()
                     << " is not equal embedding dimension " << std::endl;
          continue;
        }
        algorithm_id_to_entry_id.emplace(
            std::make_pair(current.id, current.entry));
      }
      offset = offset + batch_size;
      if (offset >= count)
      {
        break;
      }
    }
    offset = 0;
    // Load ranked items
    /**
    while (true)
    {
      std::vector<Algorithm> temp_algorithm;
      int count = 0;
      knowledgebase::getAlgorithmAccordingRanked(batch_size, offset, source_name,
                                                 true, &temp_algorithm, &count);
      LOG(DEBUG) << "offset " << offset << " limit " << batch_size << " count "
                 << count;
      for (Algorithm current : temp_algorithm)
      {
        if (current.embedding == std::nullopt)
        {
          LOG(ERROR) << "current algorithm " << current.id << " have no embedding"
                     << std::endl;
          continue;
        }
        if (current.embedding.value().size() != getCurrentEmbeddingDimension())
        {
          LOG(ERROR) << "current algorithm " << current.id << " embedding size "
                     << current.embedding.value().size()
                     << " is not equal embedding dimension " << std::endl;
          continue;
        }
        if (current.impression > 0)
        {
          continue;
        }
        algorithm_id_to_entry_id.emplace(
            std::make_pair(current.id, current.entry));
      }
      offset = offset + batch_size;
      if (offset >= count)
      {
        break;
      }
    }
    */
    return algorithm_id_to_entry_id;
  }

  std::unordered_map<std::string, ScoreWithMetadata>
  getAllEntryToPrerankSourceForCurrentSourceKnowledge()
  {
    std::unordered_map<std::string, ScoreWithMetadata> algorithm_entry_id_to_score_with_meta;
    const char *source_name = std::getenv("TERMINUS_RECOMMEND_SOURCE_NAME");

    const int batch_size = 100;

    auto get_scores = [&](bool ranked)
    {
      long long current_rank_time = getTimeStampNow();
      int offset = 0;
      while (true)
      {
        std::vector<Algorithm> temp_algorithm;
        int count = 0;
        knowledgebase::getAlgorithmAccordingRanked(batch_size, offset, source_name,
                                                   ranked, &temp_algorithm, &count);
        LOG(INFO) << "offset " << offset << " limit " << batch_size << " count "
                  << count;

        // algorithm_list.insert(algorithm_list.end(),temp_algorithm.begin(),temp_algorithm.end());
        for (Algorithm current : temp_algorithm)
        {
          if (current.prerank_score != std::nullopt)
          {
            std::optional<Entry> temp_entry =
                knowledgebase::GetEntryById(current.entry);
            if (temp_entry == std::nullopt)
            {
              LOG(INFO) << "entry [" << current.entry << "] not exist, algorithm ["
                        << current.id << "]" << std::endl;
              continue;
            }

            if (temp_entry.value().extract == false)
            {
              LOG(INFO) << "entry [" << current.entry
                        << "] not extract, algorithm [" << current.id << "]"
                        << std::endl;
              continue;
            }

            algorithm_entry_id_to_score_with_meta[current.id] =
                ScoreWithMetadata(current.prerank_score.value() * getTimeCoefficient(temp_entry.value().timestamp), current_rank_time, ScoreEnum::SCORE_PRERANK_SCORE_AND_CREATED_TIME);
          }
        }
        offset = offset + batch_size;
        if (offset >= count)
        {
          break;
        }
      }
    };
    get_scores(false);
    get_scores(true);

    LOG(INFO) << "algorithm_entry_id_to_score_with_meta "
              << algorithm_entry_id_to_score_with_meta.size() << std::endl;
    return algorithm_entry_id_to_score_with_meta;
  }

  bool judgeWhetherExtractorCompelete()
  {
    const char *source_name = std::getenv("TERMINUS_RECOMMEND_SOURCE_NAME");
    int64_t last_rank_time =
        knowledgebase::getLastRankTime(std::string(source_name));
    LOG(DEBUG) << knowledgebase::LAST_RANK_TIME << last_rank_time << std::endl;
    int64_t last_extractor_time =
        knowledgebase::getLastExtractorTime(std::string(source_name));
    LOG(DEBUG) << knowledgebase::LAST_EXTRACTOR_TIME << last_extractor_time
               << std::endl;
    if (last_extractor_time == -1)
    {
      LOG(DEBUG) << "last_extractor_time is  " << last_extractor_time
                 << " mean extractor not executed" << std::endl;
      return false;
    }

    if (last_rank_time != -1 && last_extractor_time != -1 &&
        last_rank_time > last_extractor_time)
    {
      LOG(DEBUG) << knowledgebase::LAST_RANK_TIME << " bigger than"
                 << knowledgebase::LAST_EXTRACTOR_TIME << " task top"
                 << std::endl;
      return false;
    }
    return true;
  }
  /*void rankPredict() {
    const char *source_name = std::getenv("TERMINUS_RECOMMEND_SOURCE_NAME");
    int64_t last_rank_time =
        knowledgebase::getLastRankTime(std::string(source_name));
    LOG(DEBUG) << knowledgebase::LAST_RANK_TIME << last_rank_time << std::endl;
    int64_t last_extractor_time =
        knowledgebase::getLastExtractorTime(std::string(source_name));
    LOG(DEBUG) << knowledgebase::LAST_EXTRACTOR_TIME << last_extractor_time
               << std::endl;
    if (last_extractor_time == -1) {
      LOG(DEBUG) << "last_extractor_time is  " << last_extractor_time
                 << " mean extractor not executed" << std::endl;
      return;
    }

    if (last_rank_time != -1 && last_extractor_time != -1 &&
        last_rank_time > last_extractor_time) {
      LOG(DEBUG) << knowledgebase::LAST_RANK_TIME << " bigger than"
                 << knowledgebase::LAST_EXTRACTOR_TIME << " task top"
                 << std::endl;
      return;
    }
    std::string rank_production_model_path =
        rssrank::getRankModelPath(ModelPathType::PRODUCTION);

    if (fs::exists(rank_production_model_path) == true) {
      BoosterHandle booster;
      safe_xgboost(XGBoosterCreate(NULL, 0, &booster)) safe_xgboost(
          XGBoosterLoadModel(booster, rank_production_model_path.c_str()));
      std::unordered_map<std::string, std::string> not_ranked_algorithm_to_entry =
          getNotRankedAlgorithmToEntry();
      LOG(INFO) << "not_ranked_algorithm_to_entry size "
                << not_ranked_algorithm_to_entry.size() << std::endl;
      std::vector<std::string> need_reinfer_algorithm_entry;
      for (const auto &current_item : not_ranked_algorithm_to_entry) {
        std::optional<Entry> temp_entry =
            knowledgebase::GetEntryById(current_item.second);
        if (temp_entry == std::nullopt) {
          LOG(ERROR) << "entry [" << current_item.second
                     << "] not exist, algorithm [" << current_item.first << "]"
                     << std::endl;
          continue;
        }
        if (!temp_entry.value().extract) {
          LOG(ERROR) << "entry [" << current_item.second
                     << "] not extract, algorithm [" << current_item.first << "]"
                     << std::endl;
          continue;
        }
        need_reinfer_algorithm_entry.push_back(current_item.first);
      }
      LOG(INFO) << "need_reinfer_algorithm_entry size "
                << need_reinfer_algorithm_entry.size() << std::endl;

      if (need_reinfer_algorithm_entry.size() > 0) {
        // Split into sub groups
        auto algorithm_entry_partion =
            partitionChunk(need_reinfer_algorithm_entry.begin(),
                           need_reinfer_algorithm_entry.end(), 128);

        // Display grouped batches
        for (unsigned batch_count = 0;
             batch_count < algorithm_entry_partion.size(); ++batch_count) {
          auto batch = algorithm_entry_partion[batch_count];
          std::vector<std::string> current_batch_algorithm_ids;
          for (const auto &item : batch) {
            current_batch_algorithm_ids.push_back(item);
          }

          std::vector<std::vector<float>> current_batch_feature(
              current_batch_algorithm_ids.size(),
              std::vector<float>(getCurrentEmbeddingDimension(), 0));

          fillFeaturesKnowledgeVector(current_batch_feature,current_batch_algorithm_ids,FillFeatureSource::ALGORITHM);

          std::vector<float> current_batch_feature_flatten =
              flatten(current_batch_feature);
          std::unordered_map<std::string, float> id_to_score =
              rssrank::inferPredictVector(current_batch_feature_flatten,
                                          current_batch_algorithm_ids.size(),
                                          booster, current_batch_algorithm_ids);

          LOG(DEBUG) << "id_to_score size " << id_to_score.size() << std::endl;
          // updateAlgorithmEntryRankScore(client,id_to_score);
          knowledgebase::updateAlgorithmScoreAndRanked(id_to_score);
          knowledgebase::updateLastRankTime(source_name, getTimeStampNow());
        }
      } else {
        LOG(DEBUG) << "there is no algorithm entry needed to rewrite";
      }
      safe_xgboost(XGBoosterFree(booster));
    } else {
      //
      LOG(DEBUG) << "rank production model path not exist "
                 << rank_production_model_path << std::endl;

      std::unordered_map<std::string, float> algorithm_entry_id_to_score =
          rssrank::getAllEntryToPrerankSourceForCurrentSourceKnowledge();
      knowledgebase::updateAlgorithmScoreAndRanked(algorithm_entry_id_to_score);
      LOG(ERROR) << "copy prerank score to score" << std::endl;
      knowledgebase::updateLastRankTime(source_name, getTimeStampNow());
    }
  }*/

  void
  getEachImpressionScoreKnowledge(
      std::unordered_map<std::string, float> *positive,
      std::unordered_map<std::string, float> *negative)
  {
    positive->clear();
    negative->clear();
    const char *source_name = std::getenv("TERMINUS_RECOMMEND_SOURCE_NAME");
    const int batch_size = 1000;
    int offset = 0;
    int count;
    while (true)
    {
      LOG(DEBUG) << "offset " << offset << " limit " << batch_size << " count "
                 << count;
      std::vector<Impression> temp_impressions;
      knowledgebase::getImpression(batch_size, offset, source_name,
                                   &temp_impressions, &count);
      // impression_list.insert(impression_list.end(),temp_impression.begin(),temp_impression.end());
      for (const auto &current_impression : temp_impressions)
      {
        if (current_impression.embedding == std::nullopt)
        {
          LOG(ERROR) << "current_impression " << current_impression.id
                     << " embedding not exist" << std::endl;
          continue;
        }
        size_t current_embedding_dimension =
            current_impression.embedding.value().size();
        if (current_embedding_dimension != getCurrentEmbeddingDimension())
        {
          LOG(ERROR) << "current_impression " << current_impression.id
                     << " embedding dimension size "
                     << current_embedding_dimension << " not equal to "
                     << getCurrentEmbeddingDimension() << std::endl;
          continue;
        }
        // todo filter embedding empty data
        float score = getSpecificImpressionScore(current_impression);
        if (score > 0)
        {
          positive->emplace(std::make_pair(current_impression.id, score));
        }
        else
        {
          negative->emplace(std::make_pair(current_impression.id, 0));
        }
      }
      offset = offset + batch_size;
      if (offset >= count)
      {
        break;
      }
    }
  }

  bool getCurrentBatchFromPositiveNegativeSampleWithSpeicifiedSize(
      const std::unordered_set<std::string> &train_positive_sample,
      const std::unordered_set<std::string> &train_negative_sample,
      std::vector<std::string> *current_batch, int positive_batch_size,
      int negative_batch_size)
  {
    if (train_positive_sample.size() < 10)
    {
      LOG(ERROR) << "train positive sample small than 10" << std::endl;
      return false;
    }

    if (train_negative_sample.size() < 10)
    {
      LOG(ERROR) << "train negative sample small than 10" << std::endl;
      return false;
    }
    current_batch->clear();
    std::vector<std::string> current_batch_positive;
    std::sample(train_positive_sample.begin(), train_positive_sample.end(),
                std::back_inserter(current_batch_positive), positive_batch_size,
                std::mt19937{std::random_device{}()});
    std::vector<std::string> current_batch_negative;
    std::sample(train_negative_sample.begin(), train_negative_sample.end(),
                std::back_inserter(current_batch_negative), negative_batch_size,
                std::mt19937{std::random_device{}()});
    if (current_batch_negative.size() < negative_batch_size)
    {
      RandomOverResampleSpecificNumber(negative_batch_size,
                                       &current_batch_negative);
    }
    if (current_batch_positive.size() < positive_batch_size)
    {
      RandomOverResampleSpecificNumber(positive_batch_size,
                                       &current_batch_positive);
    }

    current_batch->insert(current_batch->end(), current_batch_positive.begin(),
                          current_batch_positive.end());
    current_batch->insert(current_batch->end(), current_batch_negative.begin(),
                          current_batch_negative.end());

    auto rd = std::random_device{};
    auto rng = std::default_random_engine{rd()};
    std::shuffle(std::begin(*current_batch), std::end(*current_batch), rng);
    return true;
  }

  bool getCurrentBatchFromPositiveNegativeSample(
      const std::unordered_set<std::string> &train_positive_sample,
      const std::unordered_set<std::string> &train_negative_sample,
      std::vector<std::string> *current_batch, int positive_entry_in_one_batch)
  {
    if (train_positive_sample.size() < 10)
    {
      LOG(ERROR) << "train positive sample small than 10" << std::endl;
      return false;
    }

    if (train_negative_sample.size() < 10)
    {
      LOG(ERROR) << "train negative sample small than 10" << std::endl;
      return false;
    }
    current_batch->clear();
    std::vector<std::string> current_batch_positive;
    std::sample(train_positive_sample.begin(), train_positive_sample.end(),
                std::back_inserter(current_batch_positive),
                positive_entry_in_one_batch,
                std::mt19937{std::random_device{}()});
    std::vector<std::string> current_batch_negative;
    std::sample(train_negative_sample.begin(), train_negative_sample.end(),
                std::back_inserter(current_batch_negative),
                2 * positive_entry_in_one_batch,
                std::mt19937{std::random_device{}()});
    if (current_batch_negative.size() < 2 * positive_entry_in_one_batch)
    {
      RandomOverResampleSpecificNumber(2 * positive_entry_in_one_batch,
                                       &current_batch_negative);
    }
    current_batch->insert(current_batch->end(), current_batch_positive.begin(),
                          current_batch_positive.end());
    current_batch->insert(current_batch->end(), current_batch_negative.begin(),
                          current_batch_negative.end());

    auto rd = std::random_device{};
    auto rng = std::default_random_engine{rd()};
    std::shuffle(std::begin(*current_batch), std::end(*current_batch), rng);
    return true;
  }

  std::vector<std::unordered_map<std::string, std::string>>
  makeParameterCombination()
  {
    std::unordered_map<std::string, std::vector<std::string>> parameters;

    parameters["max_depth"] = {"3", "5", "7", "9"};
    parameters["min_child_weight"] = {"1", "3", "5", "7"};
    parameters["n_estimators"] = {"5", "10", "20"};
    parameters["learning_rate"] = {"0.1", "0.05", "0.01"};

    /**
    parameters["max_depth"] = {"9"};
    parameters["min_child_weight"] = {"7"};
    parameters["n_estimators"] = {"20"};
    parameters["learning_rate"] = {"0.1"};
    */

    std::vector<std::unordered_map<std::string, std::string>>
        combination_parameters;
    for (auto &&[max_depth, min_child_weight, n_estimators, learning_rate] :
         iter::product(parameters["max_depth"], parameters["min_child_weight"],
                       parameters["n_estimators"], parameters["learning_rate"]))
    {
      std::unordered_map<std::string, std::string> current_parameter;
      current_parameter["max_depth"] = max_depth;
      current_parameter["min_child_weight"] = min_child_weight;
      current_parameter["n_estimators"] = n_estimators;
      current_parameter["learning_rate"] = learning_rate;
      combination_parameters.push_back(current_parameter);
    }
    return combination_parameters;
  }

  bool fillFeaturesKnowledgeVector(std::vector<std::vector<float>> &features,
                                   const std::vector<std::string> &ids,
                                   enum FillFeatureSource embedding_source)
  {
    if (features.size() != ids.size())
    {
      LOG(ERROR) << "features space is not equal impress_ids rows "
                 << features.size() << " impression_id size " << ids.size()
                 << std::endl;
      return false;
    }

    std::string embedding_source_name = "";
    if (embedding_source == FillFeatureSource::ALGORITHM)
    {
      embedding_source_name = "algorithm";
    }
    else if (embedding_source = FillFeatureSource::IMPRESSION)
    {
      embedding_source_name = "impression";
    }
    LOG(DEBUG) << "embedding source name is " << embedding_source_name
               << std::endl;
    std::unordered_map<std::string, std::vector<int>> id_to_index;
    for (int index = 0; index < ids.size(); index++)
    {
      std::string current_id = ids[index];
      if (id_to_index.find(current_id) == id_to_index.end())
      {
        std::vector<int> init_vector{index};
        id_to_index[current_id] = init_vector;
      }
      else
      {
        id_to_index[current_id].push_back(index);
      }
    }

    for (std::string current_id : ids)
    {
      std::vector<double> current_embedding;
      if (embedding_source == FillFeatureSource::IMPRESSION)
      {
        std::optional<Impression> temp_impression_opt =
            knowledgebase::GetImpressionById(current_id);
        if (temp_impression_opt == std::nullopt)
        {
          LOG(ERROR) << "current " << embedding_source_name << " " << current_id
                     << " not exist" << std::endl;
          return false;
        }
        Impression temp_impression = temp_impression_opt.value();
        if (temp_impression.embedding == std::nullopt)
        {
          LOG(ERROR) << "current " << embedding_source_name << " " << current_id
                     << " have no embedding" << std::endl;
          return false;
        }
        current_embedding = temp_impression.embedding.value();
      }
      else if (embedding_source == FillFeatureSource::ALGORITHM)
      {
        std::optional<Algorithm> temp_algorithm_opt =
            knowledgebase::GetAlgorithmById(current_id);
        if (temp_algorithm_opt == std::nullopt)
        {
          LOG(ERROR) << "current " << embedding_source_name << " " << current_id
                     << " not exist" << std::endl;
          return false;
        }
        Algorithm temp_algorithm = temp_algorithm_opt.value();
        if (temp_algorithm.embedding == std::nullopt)
        {
          LOG(ERROR) << "current " << embedding_source_name << " " << current_id
                     << " have no embedding" << std::endl;
          return false;
        }
        current_embedding = temp_algorithm.embedding.value();
      }

      if (current_embedding.size() != getCurrentEmbeddingDimension())
      {
        LOG(ERROR) << embedding_source_name << " " << current_id
                   << " embedding size " << current_embedding.size()
                   << " not equal current embedding dimension "
                   << features[0].size() << std::endl;
        return false;
      }

      for (const auto &index : id_to_index[current_id])
      {
        std::copy(current_embedding.begin(), current_embedding.end(),
                  std::begin(features[index]));
      }
    }
    return true;
  }

  std::unordered_map<std::string, float> inferPredictVector(
      const std::vector<float> &test_features, int test_rows,
      BoosterHandle h_booster, const std::vector<std::string> &id_list)
  {
    std::unordered_map<std::string, float> id_to_score;
    if (id_list.size() != test_rows)
    {
      LOG(ERROR) << "id_list size " << id_list.size() << " not equal rows "
                 << test_rows << std::endl;
      return id_to_score;
    }
    DMatrixHandle h_test;
    XGDMatrixCreateFromMat(test_features.data(), test_rows,
                           getCurrentEmbeddingDimension(), -1, &h_test);
    bst_ulong out_len;
    const float *f;
    XGBoosterPredict(h_booster, h_test, 0, 0, 0, &out_len, &f);

    for (unsigned int i = 0; i < out_len; i++)
    {
      id_to_score[id_list[i]] = f[i];
    }
    XGDMatrixFree(h_test);
    return id_to_score;
  }

  bool trainOneBigBatchPredictVector(const std::vector<float> &test_features,
                                     int test_rows, float *test_labels,
                                     BoosterHandle h_booster,
                                     float *biggest_auc)
  {
    DMatrixHandle h_test;
    XGDMatrixCreateFromMat(test_features.data(), test_rows,
                           getCurrentEmbeddingDimension(), -1, &h_test);
    bst_ulong out_len;
    const float *f;
    XGBoosterPredict(h_booster, h_test, 0, 0, 0, &out_len, &f);
    float total_positive = 0;

    float auc = AUROC<float, float>(test_labels, f, test_rows);
    if (auc > *biggest_auc)
    {
      *biggest_auc = auc;
      LOG(DEBUG) << "current biggest_auc " << *biggest_auc << std::endl;
      std::string rank_training_model_path =
          rssrank::getRankModelPath(ModelPathType::TRAINING);
      safe_xgboost(
          XGBoosterSaveModel(h_booster, rank_training_model_path.c_str()));
    }

    XGDMatrixFree(h_test);
    return true;
  }

  bool trainOneBigBatchWithPreparedDataVector(
      const std::vector<float> &features, float *train_labels, const int rows,
      const std::unordered_map<std::string, std::string> &parameters,
      const std::vector<float> &test_features, int test_rows, float *test_labels,
      float *biggest_auc)
  {
    // convert to DMatrix
    DMatrixHandle h_train[1];
    safe_xgboost(XGDMatrixCreateFromMat(
        features.data(), rows, getCurrentEmbeddingDimension(), -1, &h_train[0]));

    // load the labels
    safe_xgboost(XGDMatrixSetFloatInfo(h_train[0], "label", train_labels, rows));

    BoosterHandle h_booster;

    safe_xgboost(XGBoosterCreate(h_train, 1, &h_booster));
    const char *validation_names[1] = {"train"};
    const char *validation_result = NULL;
    XGBoosterSetParam(h_booster, "objective", "binary:logistic");
    for (const auto &current_parameter : parameters)
    {
      safe_xgboost(XGBoosterSetParam(h_booster, current_parameter.first.c_str(),
                                     current_parameter.second.c_str()));
    }

    for (int iter = 0; iter < 100; iter++)
    {
      safe_xgboost(XGBoosterUpdateOneIter(h_booster, iter, h_train[0]));
      trainOneBigBatchPredictVector(test_features, test_rows, test_labels,
                                    h_booster, biggest_auc);
    }

    XGDMatrixFree(h_train[0]);
    XGBoosterFree(h_booster);
  }

  vector<FeatureExtractor *> initLRFeatureExtractors()
  {
    return {new EmbeddingDistanceExtractor()};
  }

  bool rankLR()
  {
    std::string current_srouce_name = envOrBlank("TERMINUS_RECOMMEND_SOURCE_NAME");
    if (FLAGS_recommend_source_name.size() == 0)
    {
      LOG(ERROR) << "recommend_source_name not provided." << std::endl;
      return false;
    }

    knowledgebase::EntryCache::getInstance().init();
    bool result = doRank();
    if (!result)
    {
      std::unordered_map<std::string, ScoreWithMetadata> entry_to_score_with_metadata =
          rssrank::getAllEntryToPrerankSourceForCurrentSourceKnowledge();
      knowledgebase::updateAlgorithmScoreAndMetadata(entry_to_score_with_metadata);
      LOG(ERROR) << "falling back to prerank score." << std::endl;
      knowledgebase::updateLastRankTime(FLAGS_recommend_source_name, getTimeStampNow());
    }

    return true;
  }

  bool rankByTimeForColdStart(long long current_rank_time, const vector<Impression> &clicked_impressions)
  {
    // long long current_rank_time = getTimeStampNow();
    std::string current_srouce_name = envOrBlank("TERMINUS_RECOMMEND_SOURCE_NAME");
    std::vector<int> all_source_rank_time = knowledgebase::findAllRecomendTraceInfoRankTimesBySource(current_srouce_name);
    std::optional<RecommendTraceInfo> previous_recommend_trace_info = std::nullopt;
    if (all_source_rank_time.size() > 0)
    {
      previous_recommend_trace_info = knowledgebase::findRecommendTraceInfoByRankTimeAndSource(current_srouce_name, all_source_rank_time[0]);
    }

    std::unordered_map<std::string, std::string> not_impressioned_algorithm_to_entry =
        getNotImpressionedAlgorithmToEntry();
    std::unordered_map<std::string, ScoreWithMetadata> id_to_score_with_meta;
    std::vector<int> not_impressioned_algorithm_integer_ids;
    std::vector<pair<int, double>> algorithm_integer_id_to_score;
    int embedding_dimension = getCurrentEmbeddingDimension();
    std::vector<double> recall_user_embedding = knowledgebase::init_user_embedding(embedding_dimension);
    knowledgebase::updateRecallUserEmbedding(current_srouce_name, recall_user_embedding, current_rank_time);
    for (const auto &current_item : not_impressioned_algorithm_to_entry)
    {
      std::optional<Entry> temp_entry =
          knowledgebase::EntryCache::getInstance().getEntryById(current_item.second);
      if (temp_entry == std::nullopt)
      {
        LOG(ERROR) << "entry [" << current_item.second
                   << "] not exist, algorithm [" << current_item.first << "]"
                   << std::endl;
        continue;
      }
      if (!temp_entry.value().extract)
      {
        LOG(ERROR) << "entry [" << current_item.second
                   << "] not extract, algorithm [" << current_item.first << "]"
                   << std::endl;
        continue;
      }
      std::optional<Algorithm> current_algorithm =
          knowledgebase::GetAlgorithmById(current_item.first);
      if (current_algorithm == std::nullopt)
      {
        LOG(ERROR) << "Algorithm item not found, id = " << current_item.first << std::endl;
        continue;
      }
      not_impressioned_algorithm_integer_ids.push_back(current_algorithm.value().integer_id);

      double time_score = 0;
      if (temp_entry.value().published_at != 0)
      {
        time_score = getTimeCoefficientForUnixTimestamp(temp_entry.value().published_at, current_rank_time);
      }
      else
      {
        time_score = getTimeCoefficient(temp_entry.value().timestamp);
      }
      id_to_score_with_meta.emplace(current_item.first, buildScoreWithMeta(time_score, "", current_rank_time, ScoreEnum::SCORE_PUBLISHED_AT_TIME));
      algorithm_integer_id_to_score.push_back(std::make_pair(current_algorithm.value().integer_id, time_score));
    }

    std::sort(algorithm_integer_id_to_score.begin(), algorithm_integer_id_to_score.end(), [](const std::pair<int, double> &a, const std::pair<int, double> &b)
              { return a.second > b.second; });
    if (FLAGS_verbose)
    {
      std::vector<std::pair<std::string, float>> sorted_algorithm_to_score = knowledgebase::rankScoreMetadata(id_to_score_with_meta);
      for (const auto &pr : sorted_algorithm_to_score)
      {
        LOG(INFO) << "Algorithm [" << pr.first << "] score [" << pr.second << "]" << std::endl;
      }
      std::cout << "max " << sorted_algorithm_to_score[0].second << " min " << sorted_algorithm_to_score[sorted_algorithm_to_score.size() - 1].second << std::endl;
    }
    if (FLAGS_upload_score)
    {
      LOG(INFO) << "score with metadata update to knowledge " << std::endl;
      knowledgebase::updateAlgorithmScoreAndMetadataWithScoreOrder(id_to_score_with_meta);
      knowledgebase::updateLastRankTime(FLAGS_recommend_source_name, current_rank_time);
      RecommendTraceInfo current_recommend_trace_info = buildRecommendTraceInfo(previous_recommend_trace_info, current_srouce_name, current_rank_time,
                                                                                ScoreEnum::SCORE_PUBLISHED_AT_TIME,
                                                                                not_impressioned_algorithm_integer_ids,
                                                                                getIntegerIdFromVecImpression(clicked_impressions),
                                                                                "",
                                                                                "",
                                                                                "",
                                                                                algorithm_integer_id_to_score,
                                                                                globalTerminusRecommendParams);
      knowledgebase::postRecommendTraceInfo(current_recommend_trace_info);
    }
    return true;
  }

  bool rankShortTermAndLongTermUserEmbedding()
  {
    knowledgebase::init_global_terminus_recommend_params();
    if (!judgeWhetherExtractorCompelete())
    {
      LOG(ERROR) << "extractor not complete" << std::endl;
      return false;
    }
    long long current_rank_time = getTimeStampNow();
    std::string current_srouce_name = envOrBlank("TERMINUS_RECOMMEND_SOURCE_NAME");
    if (FLAGS_recommend_source_name.size() == 0)
    {
      LOG(ERROR) << "recommend_source_name not provided." << std::endl;

      return false;
    }

    LOG(INFO)
        << "recommend_source_name " << FLAGS_recommend_source_name << std::endl;

    // int cold_start_article_clicked_number = getEnvInt(TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD, 10);
    int cold_start_article_clicked_number = globalTerminusRecommendParams.cold_start_article_clicked_number_threshold;
    knowledgebase::EntryCache::getInstance().init();
    vector<Impression> clicked_impressions = getImpressionForShortTermAndLongTermUserEmbeddingRank();
    // TODO: If the number of clicked articles is less than 10, do not use the recommendation algorithm. Since they are all in one category, sort by time for cold start.
    if (clicked_impressions.size() < cold_start_article_clicked_number)
    {
      LOG(INFO) << "clicked_impressions size " << clicked_impressions.size() << " less than " << cold_start_article_clicked_number << std::endl;
      rankByTimeForColdStart(current_rank_time, clicked_impressions);
      return true;
    }
    std::vector<int> all_source_rank_time = knowledgebase::findAllRecomendTraceInfoRankTimesBySource(current_srouce_name);
    std::optional<RecommendTraceInfo> previous_recommend_trace_info = std::nullopt;
    if (all_source_rank_time.size() > 0)
    {
      previous_recommend_trace_info = knowledgebase::findRecommendTraceInfoByRankTimeAndSource(current_srouce_name, all_source_rank_time[0]);
    }

    int embedding_dimension = getCurrentEmbeddingDimension();

    // int short_number = getEnvInt(TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION, 10);
    int short_number = globalTerminusRecommendParams.short_term_user_embedding_impression_count;
    // float long_term_weight = getEnvFloat(TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE, 0.3);
    float long_term_weight = globalTerminusRecommendParams.long_term_user_embedding_weight_for_rankscore;
    float short_term_weight = 1 - long_term_weight; // long_term_weight + short_term_weight = 1
    // float article_time_weight = getEnvFloat(TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE, 0.5);
    float article_time_weight = globalTerminusRecommendParams.article_time_weight_for_rankscore;

    vector<Impression> short_term_impression = get_subvector(clicked_impressions, short_number);
    vector<double> short_term_embedding = calcluateUserShortTermEmbedding(short_term_impression, true);

    // todo after calculate shore term embedding, clear short_term_impression
    if (short_term_embedding.size() == 0)
    {
      short_term_embedding = vector<double>(embedding_dimension, 0.0);
    }
    // Eigen::VectorXd short_term_vectorxd = vectorToEigentVectorXd(short_term_embedding);

    // post short term user embedding
    RecommendTraceUserEmbedding short_recommend_trace_user_embedding =
        buildRecommendTraceUserEmbedding(current_srouce_name, short_term_embedding, getIntegerIdFromVecImpression(short_term_impression), current_rank_time);
    knowledgebase::postRecommendTraceUserEmbedding(short_recommend_trace_user_embedding);

    // vector<double> long_term_embedding = knowledgebase::getRecallUserEmbedding(current_srouce_name);
    vector<double> long_term_embedding = calcluateUserLongTermEmbedding(clicked_impressions);

    // post long term user embedding
    RecommendTraceUserEmbedding long_recommend_trace_user_embedding =
        buildRecommendTraceUserEmbedding(current_srouce_name, long_term_embedding, getIntegerIdFromVecImpression(clicked_impressions), current_rank_time);
    knowledgebase::postRecommendTraceUserEmbedding(long_recommend_trace_user_embedding);

    // std::string recall_embedding = getEnvString(TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING, "long");
    std::string recall_embedding = globalTerminusRecommendParams.long_or_short_embedding_as_recall_embedding;
    std::string current_recall_user_embedding_id = "";
    if (recall_embedding == "long")
    {
      current_recall_user_embedding_id = long_recommend_trace_user_embedding.unique_id;
      knowledgebase::updateRecallUserEmbedding(current_srouce_name, long_term_embedding, current_rank_time);
    }
    else if (recall_embedding == "short")
    {
      current_recall_user_embedding_id = short_recommend_trace_user_embedding.unique_id;
      knowledgebase::updateRecallUserEmbedding(current_srouce_name, short_term_embedding, current_rank_time);
    }
    else
    {
      LOG(ERROR) << "recall_embedding " << recall_embedding << " not support" << std::endl;
      return false;
    }

    // Eigen::VectorXd long_term_vectorxd = vectorToEigentVectorXd(long_term_embedding);

    std::unordered_map<std::string, std::string> not_impressioned_algorithm_to_entry =
        getNotImpressionedAlgorithmToEntry(); // This is actually algorithm_id_to_entry_id, the current logic does not distinguish between ranked and unranked, it is all algorithms under a certain source
    LOG(INFO) << "not_impressioned_algorithm_to_entry size "
              << not_impressioned_algorithm_to_entry.size() << std::endl;
    std::vector<std::string> need_reinfer_algorithm_entry;
    std::unordered_map<std::string, ScoreWithMetadata> id_to_score_with_meta;
    long long current_time = getTimeStampNow();
    FAISSArticleSearch search(clicked_impressions);

    std::vector<int> not_impressioned_algorithm_integer_ids;
    std::vector<pair<int, double>> algorithm_integer_id_to_score;
    for (const auto &current_item : not_impressioned_algorithm_to_entry)
    {
      std::optional<Entry> temp_entry =
          knowledgebase::EntryCache::getInstance().getEntryById(current_item.second);
      if (temp_entry == std::nullopt)
      {
        LOG(ERROR) << "entry [" << current_item.second
                   << "] not exist, algorithm [" << current_item.first << "]"
                   << std::endl;
        continue;
      }
      if (!temp_entry.value().extract)
      {
        LOG(ERROR) << "entry [" << current_item.second
                   << "] not extract, algorithm [" << current_item.first << "]"
                   << std::endl;
        continue;
      }
      std::optional<Algorithm> current_algorithm =
          knowledgebase::GetAlgorithmById(current_item.first);
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
      not_impressioned_algorithm_integer_ids.push_back(current_algorithm.value().integer_id);
      vector<double> current_article_embedding = current_algorithm.value().embedding.value();
      // Eigen::VectorXd current_article_vectorxd = vectorToEigentVectorXd(current_article_embedding);
      double short_term_similarity_score = normalized_similarity_score_based_on_cosine_similarity(current_article_embedding, short_term_embedding);
      double long_term_similarity_score = normalized_similarity_score_based_on_cosine_similarity(current_article_embedding, long_term_embedding);
      double similarity_score = short_term_weight * short_term_similarity_score +
                                long_term_weight * long_term_similarity_score;
      double time_score = 0;
      if (temp_entry.value().published_at != 0)
      {
        time_score = getTimeCoefficientForUnixTimestamp(temp_entry.value().published_at, current_time);
      }
      else
      {
        time_score = getTimeCoefficient(temp_entry.value().timestamp);
      }
      double score = similarity_score * (1 - article_time_weight) + article_time_weight * time_score;
      LOG(INFO) << "current_item [" << temp_entry.value().title << "] score [" << score << "]" << std::endl;
      std::pair<int, float> row_id_to_distance = search.findMostSimilarArticle(current_algorithm.value().embedding.value());
      std::string most_similar_article_id = clicked_impressions[row_id_to_distance.first].entry_id;
      id_to_score_with_meta.emplace(current_item.first, buildScoreWithMeta(score, most_similar_article_id, current_rank_time, ScoreEnum::SCORE_DISTANCE_ARTICLE_WITH_SHORT_AND_LONG_TERM_USER_EMBEDDING_PLUS_PUBLISHED_TIME));
      algorithm_integer_id_to_score.push_back(std::make_pair(current_algorithm.value().integer_id, score));
    }
    std::sort(algorithm_integer_id_to_score.begin(), algorithm_integer_id_to_score.end(), [](const std::pair<int, double> &a, const std::pair<int, double> &b)
              { return a.second > b.second; });

    if (FLAGS_verbose)
    {
      std::vector<std::pair<std::string, float>> sorted_algorithm_to_score = knowledgebase::rankScoreMetadata(id_to_score_with_meta);
      for (const auto &pr : sorted_algorithm_to_score)
      {
        LOG(INFO) << "Algorithm [" << pr.first << "] score [" << pr.second << "]" << std::endl;
      }
      std::cout << "max " << sorted_algorithm_to_score[0].second << " min " << sorted_algorithm_to_score[sorted_algorithm_to_score.size() - 1].second << std::endl;
    }
    if (FLAGS_upload_score)
    {
      LOG(INFO) << "score with metadata update to knowledge " << std::endl;
      knowledgebase::updateAlgorithmScoreAndMetadataWithScoreOrder(id_to_score_with_meta);
      knowledgebase::updateLastRankTime(FLAGS_recommend_source_name, current_rank_time);
      RecommendTraceInfo current_recommend_trace_info = buildRecommendTraceInfo(previous_recommend_trace_info, current_srouce_name, current_rank_time,
                                                                                ScoreEnum::SCORE_DISTANCE_ARTICLE_WITH_SHORT_AND_LONG_TERM_USER_EMBEDDING_PLUS_PUBLISHED_TIME,
                                                                                not_impressioned_algorithm_integer_ids,
                                                                                getIntegerIdFromVecImpression(clicked_impressions),
                                                                                long_recommend_trace_user_embedding.unique_id,
                                                                                short_recommend_trace_user_embedding.unique_id,
                                                                                current_recall_user_embedding_id,
                                                                                algorithm_integer_id_to_score,
                                                                                globalTerminusRecommendParams);
      knowledgebase::postRecommendTraceInfo(current_recommend_trace_info);
    }

    return true;
  }

  bool doRank()
  {
    long long current_rank_time = getTimeStampNow();
    std::unique_ptr<lr::LogisticRegression> logistic_regression;
    if (FLAGS_model_path_root.size() != 0)
    {
      auto model_file = FLAGS_model_path_root + "/lr.model";
      if (fs::exists(model_file))
      {
        lr::SimpleModelSerializer serializer;
        lr::LogisticRegression *model;
        if (!serializer.loadModel(model_file, &model))
        {
          LOG(ERROR) << "Failed to load model file." << endl;
          return false;
        }
        logistic_regression.reset(model);
      }
      else
      {
        LOG(ERROR) << "Failed to load model file: " << model_file << endl;
        return false;
      }
    }
    else
    { // Load the default model, since currently only one feature is present the model is not important
      logistic_regression = loadDefaultModel();
    }

    for (auto weight : logistic_regression->weights)
    {
      LOG(INFO) << "LR model weights: " << weight << std::endl;
    }
    LOG(INFO) << "LR model loaded successfully." << std::endl;

    // TODO(haochengwang): Sort the impressions by event time
    auto feature_extractors = initLRFeatureExtractors();
    auto impressions = getImpressionsForLR();

    for (const auto &impression : impressions)
    {
      for (auto extractor : feature_extractors)
      {
        extractor->addSample(impression);
      }
    }
    LOG(INFO) << "Feature extractors initialized." << std::endl;
    for (auto extractor : feature_extractors)
    {
      if (!extractor->ready())
      {
        LOG(ERROR) << "Not all the feature extractor are ready.";
        return false;
      }
    }
    LOG(INFO) << "Feature extractors are ready." << std::endl;
    std::unordered_map<std::string, std::string> not_ranked_algorithm_to_entry =
        getNotRankedAlgorithmToEntry();
    LOG(INFO) << "not_ranked_algorithm_to_entry size "
              << not_ranked_algorithm_to_entry.size() << std::endl;
    std::vector<std::string> need_reinfer_algorithm_entry;
    std::unordered_map<std::string, ScoreWithMetadata> id_to_score_with_meta;
    for (const auto &current_item : not_ranked_algorithm_to_entry)
    {
      std::optional<Entry> temp_entry =
          knowledgebase::EntryCache::getInstance().getEntryById(current_item.second);
      if (temp_entry == std::nullopt)
      {
        LOG(ERROR) << "entry [" << current_item.second
                   << "] not exist, algorithm [" << current_item.first << "]"
                   << std::endl;
        continue;
      }
      if (!temp_entry.value().extract)
      {
        LOG(ERROR) << "entry [" << current_item.second
                   << "] not extract, algorithm [" << current_item.first << "]"
                   << std::endl;
        continue;
      }
      std::optional<Algorithm> current_algorithm =
          knowledgebase::GetAlgorithmById(current_item.first);
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

      vector<double> features(feature_extractors.size(), 0.0);
      for (int i = 0; i < feature_extractors.size(); ++i)
      {
        features[i] = feature_extractors[i]->extract(current_algorithm.value());
      }

      auto score = logistic_regression->predict(features);
      score *= getTimeCoefficient(temp_entry.value().timestamp);
      if (FLAGS_verbose)
      {
        LOG(INFO) << "Score: " << score << endl;
      }
      auto reason = feature_extractors[0]->getReason(current_algorithm.value());
      id_to_score_with_meta.emplace(current_item.first, buildScoreWithMeta(score, reason.reason, current_rank_time, ScoreEnum::SCORE_LOGISTIC_REGRESSION_AND_CREATED_TIME));
    }

    LOG(INFO) << "id_to_score_with_meta size " << id_to_score_with_meta.size() << std::endl;
    knowledgebase::EntryCache::getInstance().dumpStatistics();
    if (FLAGS_verbose)
    {
      for (const auto &pr : id_to_score_with_meta)
      {
        LOG(INFO) << pr.first << "->" << pr.second << std::endl;
      }
    }
    if (FLAGS_upload_score)
    {
      knowledgebase::updateAlgorithmScoreAndMetadata(id_to_score_with_meta);
      knowledgebase::updateLastRankTime(FLAGS_recommend_source_name, getTimeStampNow());
    }
    return true;
  }

  bool trainLR()
  {
    const char *source_name = std::getenv("TERMINUS_RECOMMEND_SOURCE_NAME");
    auto impressions = getImpressionsForLR();

    // TODO(haochengwang): Sort the impressions by event time
    auto feature_extractors = initLRFeatureExtractors();
    lr::LogisticRegression logistic_regression(feature_extractors.size());

    vector<vector<double>> pos, neg;
    for (const auto &impression : impressions)
    {
      bool available = true;
      vector<double> f;
      for (auto extractor : feature_extractors)
      {
        if (extractor->ready())
        {
          f.push_back(extractor->extract(impression));
        }
        else
        {
          available = false;
        }
      }

      if (available)
      {
        if (impression.clicked)
        {
          pos.push_back(f);
        }
        else
        {
          neg.push_back(f);
        }
      }

      for (auto extractor : feature_extractors)
      {
        extractor->addSample(impression);
      }
    }
    logistic_regression.fit(pos, neg, 0.1, 1000);

    for (auto weight : logistic_regression.weights)
    {
      LOG(INFO) << "LR weight: " << weight << std::endl;
    }

    lr::SimpleModelSerializer serializer;
    if (!serializer.saveModel(FLAGS_model_path_root + "/lr.model", logistic_regression))
    {
      LOG(ERROR) << "Failed to save model." << endl;
      return false;
    }

    return true;
  }

  bool trainOneBigBatch()
  {
    // to do
    std::unordered_map<std::string, float> positive;
    std::unordered_map<std::string, float> negative;

    getEachImpressionScoreKnowledge(&positive, &negative);
    LOG(DEBUG) << "positive impression " << positive.size()
               << "negative impression " << negative.size() << std::endl;
    std::unordered_set<std::string> positive_impression_id_set;
    std::unordered_set<std::string> negative_impression_id_set;

    for (const auto &current_item : positive)
    {
      positive_impression_id_set.insert(current_item.first);
    }

    for (const auto &current_item : negative)
    {
      negative_impression_id_set.insert(current_item.first);
    }

    std::unordered_set<std::string> train_positive_sample;
    std::unordered_set<std::string> train_negative_sample;
    std::unordered_set<std::string> test_positive_sample;
    std::unordered_set<std::string> test_negative_sample;
    bool configure_result = ConfigureTrainTestRatioWithoutOverSample(
        positive_impression_id_set, negative_impression_id_set,
        &train_positive_sample, &train_negative_sample, &test_positive_sample,
        &test_negative_sample, 0.8);
    if (configure_result == false)
    {
      LOG(DEBUG) << "train fail, for confiture train test ratio fial"
                 << std::endl;
      return false;
    }

    LOG(DEBUG) << "train_positive_sample " << train_positive_sample.size()
               << " train_negative_sample " << train_negative_sample.size()
               << "test_positive_sample " << test_positive_sample.size()
               << "test_negative_sample " << test_negative_sample.size()
               << std::endl;
    // https://cloud.tencent.com/developer/article/1656126

    std::vector<std::unordered_map<std::string, std::string>>
        combination_parameter = makeParameterCombination();

    // prepare train batch data
    int train_positive_sample_limit = 1000;
    const char *env_train_positive_sample_limit =
        std::getenv("TRAIN_POSITIVE_SAMPLE_LIMIT");
    if (env_train_positive_sample_limit != nullptr &&
        isConvertibleToInt(std::string(env_train_positive_sample_limit)) &&
        std::stoi(std::string(env_train_positive_sample_limit)) >= 10)
    {
      train_positive_sample_limit =
          std::stoi(std::string(env_train_positive_sample_limit));
    }
    std::vector<std::string> batch_data;
    if (train_positive_sample.size() < train_positive_sample_limit)
    {
      train_positive_sample_limit = train_positive_sample.size();
    }
    LOG(DEBUG) << "train_positive_sample_limit " << train_positive_sample_limit
               << std::endl;
    int sample_number = train_positive_sample_limit * 3;
    bool get_current_batch_result = getCurrentBatchFromPositiveNegativeSample(
        train_positive_sample, train_negative_sample, &batch_data,
        train_positive_sample_limit);
    if (batch_data.size() != sample_number)
    {
      LOG(ERROR) << "current batch data is not equal sample number" << std::endl;
      return false;
    }
    if (get_current_batch_result == false)
    {
      LOG(ERROR) << "get current batch data fail" << std::endl;
      return false;
    }

    std::vector<std::vector<float>> feature(
        sample_number, std::vector<float>(getCurrentEmbeddingDimension(), 0));
    float train_labels[sample_number];

    std::fill(train_labels, train_labels + sample_number, -1);

    bool fill_features_result = fillFeaturesKnowledgeVector(
        feature, batch_data, FillFeatureSource::IMPRESSION);
    if (fill_features_result == false)
    {
      LOG(ERROR) << "fill features fail" << std::endl;
      return false;
    }
    LOG(DEBUG) << "compelete fill features" << std::endl;
    for (int i = 0; i < sample_number; i++)
    {
      std::string current_impression_id = batch_data[i];
      if (positive.find(current_impression_id) != positive.end())
      {
        // train_labels[i] = positive[current_impression_id];
        train_labels[i] = 1;
      }

      if (negative.find(current_impression_id) != negative.end())
      {
        // train_labels[i] = negative[current_impression_id];
        train_labels[i] = 0;
      }

      if (train_labels[i] == -1)
      {
        LOG(ERROR) << "current impression id " << current_impression_id
                   << " have no score exist" << std::endl;
        return false;
      }
    }

    // prepare test batch data
    int test_sample_limit = static_cast<int>(sample_number * 1.0 / 4);
    LOG(DEBUG) << "test_sample_limit " << test_sample_limit << std::endl;
    std::vector<std::string> test_all_batch_data;
    test_all_batch_data.insert(test_all_batch_data.end(),
                               test_positive_sample.begin(),
                               test_positive_sample.end());
    test_all_batch_data.insert(test_all_batch_data.end(),
                               test_negative_sample.begin(),
                               test_negative_sample.end());
    auto rd = std::random_device{};
    auto rng = std::default_random_engine{rd()};
    std::shuffle(std::begin(test_all_batch_data), std::end(test_all_batch_data),
                 rng);

    std::vector<std::string> practical_test_bach;
    if (test_all_batch_data.size() > test_sample_limit)
    {
      std::sample(test_all_batch_data.begin(), test_all_batch_data.end(),
                  std::back_inserter(practical_test_bach), test_sample_limit,
                  std::mt19937{std::random_device{}()});
    }
    else
    {
      practical_test_bach = test_all_batch_data;
    }

    std::vector<std::vector<float>> test_feature(
        practical_test_bach.size(),
        std::vector<float>(getCurrentEmbeddingDimension(), 0));
    float test_labels[practical_test_bach.size()];

    std::fill(test_labels, test_labels + practical_test_bach.size(), -1);

    bool fill_test_features_result = fillFeaturesKnowledgeVector(
        test_feature, practical_test_bach, FillFeatureSource::IMPRESSION);
    if (fill_test_features_result == false)
    {
      LOG(ERROR) << "fill test features fail" << std::endl;
      return false;
    }
    LOG(INFO) << "compelete fill test features" << std::endl;
    for (int i = 0; i < practical_test_bach.size(); i++)
    {
      std::string current_impression_id = practical_test_bach[i];
      if (positive.find(current_impression_id) != positive.end())
      {
        test_labels[i] = 1;
      }

      if (negative.find(current_impression_id) != negative.end())
      {
        test_labels[i] = 0;
      }

      if (test_labels[i] == -1)
      {
        LOG(ERROR) << "current impression id " << current_impression_id
                   << " have no score exist" << std::endl;
        return false;
      }
    }

    // prepare test features,labels

    LOG(DEBUG) << "compelete fill labels" << std::endl;
    float biggest_auc = -1000000;
    std::vector<float> feature_flattened = flatten(feature);
    std::vector<float> test_feature_flattened = flatten(test_feature);
    for (auto current_parameter : combination_parameter)
    {
      trainOneBigBatchWithPreparedDataVector(
          feature_flattened, train_labels, sample_number, current_parameter,
          test_feature_flattened, practical_test_bach.size(), test_labels,
          &biggest_auc);
    }
    LOG(INFO) << "biggest_auc " << biggest_auc << std::endl;
    bool copy_develop_rank_model_to_production = false;
    std::string rank_training_model_path =
        getRankModelPath(ModelPathType::TRAINING);
    std::string rank_production_model_path =
        getRankModelPath(ModelPathType::PRODUCTION);
    copy_develop_rank_model_to_production =
        (fs::exists(rank_training_model_path) == true) &&
        (fs::exists(rank_production_model_path) == false);

    if (copy_develop_rank_model_to_production == false)
    {
      copy_develop_rank_model_to_production =
          (fs::exists(rank_training_model_path) == true) &&
          (fs::exists(rank_production_model_path) == true &&
           (fs::last_write_time(rank_production_model_path) <
            fs::last_write_time(rank_training_model_path)));
      copy_develop_rank_model_to_production = true;
      LOG(INFO) << "need copy newer model" << std::endl;
    }

    if (copy_develop_rank_model_to_production)
    {
      if (fs::exists(rank_production_model_path))
      {
        fs::remove(rank_production_model_path);
      }
      fs::copy_file(rank_training_model_path, rank_production_model_path);
      LOG(INFO) << "copy rank training model file " << rank_training_model_path
                << " to " << rank_production_model_path << std::endl;
    }
  }

  int getCurrentEmbeddingDimension()
  {
    const char *embedding_dimension =
        std::getenv(TERMINUS_RECOMMEND_EMBEDDING_DIMENSION);
    if (embedding_dimension == nullptr)
    {
      LOG(ERROR) << TERMINUS_RECOMMEND_EMBEDDING_DIMENSION << " NOT EXIST"
                 << std::endl;
      exit(-1);
    }
    // LOG(INFO) << TERMINUS_RECOMMEND_EMBEDDING_DIMENSION << embedding_dimension
    //          << std::endl;
    int number = -1;
    try
    {
      number = std::stoi(embedding_dimension);
      // LOG(INFO) << "embedding_dimension is " << number << std::endl;
    }
    catch (const std::invalid_argument &e)
    {
      LOG(ERROR) << "convert embedding dimension fail" << std::endl;
    }
    catch (const std::out_of_range &e)
    {
      LOG(ERROR) << "convert embedding dimension fail" << std::endl;
    }
    if (number == -1)
    {
      exit(-1);
    }
    return number;
  }

  RecommendTraceUserEmbedding buildRecommendTraceUserEmbedding(
      const std::string &source,
      const std::vector<double> &embedding,
      const std::vector<int> &impression_id_used_to_calculate_embedding,
      const long long &created_rank_time)
  {
    RecommendTraceUserEmbedding recommend_trace_user_embedding;
    recommend_trace_user_embedding.source = source;
    recommend_trace_user_embedding.user_embedding = embedding;
    recommend_trace_user_embedding.impression_id_used_to_calculate_embedding =
        arrayToString(impression_id_used_to_calculate_embedding);
    recommend_trace_user_embedding.calculateUniqueId();
    recommend_trace_user_embedding.created_rank_time = created_rank_time;
    return recommend_trace_user_embedding;
  }

  RecommendTraceInfo buildRecommendTraceInfo(
      const std::optional<RecommendTraceInfo> &previous_recommend_trace_info,
      const std::string &source,
      const long long &rank_time,
      const ScoreEnum current_score_enum,
      const std::vector<int> &not_impressioned_algorithm_id,
      const std::vector<int> &impressioned_clicked_id,
      const std::string &long_term_user_embedding_id,
      const std::string &short_term_user_embedding_id,
      const std::string &recall_user_embedding_id,
      const std::vector<std::pair<int, double>> &algorithm_integer_id_to_score,
      const TerminusRecommendParams &recommendParams)
  {
    RecommendTraceInfo recommend_trace_info;
    recommend_trace_info.source = source;
    recommend_trace_info.rank_time = rank_time;
    recommend_trace_info.score_enum = scoreEnumToString[current_score_enum];
    recommend_trace_info.not_impressioned_algorithm_id =
        arrayToString(not_impressioned_algorithm_id);
    recommend_trace_info.impressioned_clicked_id = arrayToString(impressioned_clicked_id);
    recommend_trace_info.long_term_user_embedding_id = long_term_user_embedding_id;
    recommend_trace_info.short_term_user_embedding_id = short_term_user_embedding_id;

    std::vector<int> top_ranked_algorithm_integer_id;
    std::vector<float> top_ranked_algorithm_score;
    for (const auto &current_item : algorithm_integer_id_to_score)
    {
      top_ranked_algorithm_integer_id.push_back(current_item.first);
      top_ranked_algorithm_score.push_back(current_item.second);
    }

    recommend_trace_info.top_ranked_algorithm_id = top_ranked_algorithm_integer_id;
    recommend_trace_info.top_ranked_algorithm_score = top_ranked_algorithm_score;
    if (previous_recommend_trace_info != std::nullopt)
    {
      RecommendTraceInfo previous_recommend_trace_info_value = previous_recommend_trace_info.value();
      std::vector<int> previous_not_impressioned_algorithm_id =
          stringToArray(previous_recommend_trace_info_value.not_impressioned_algorithm_id);
      std::vector<int> added_not_impressioned_algorithm_id = find_elements_in_b_not_in_a(previous_not_impressioned_algorithm_id, not_impressioned_algorithm_id);
      recommend_trace_info.added_not_impressioned_algorithm_id = arrayToString(added_not_impressioned_algorithm_id);
      std::vector<int> previous_impressioned_clicked_id = stringToArray(previous_recommend_trace_info_value.impressioned_clicked_id);
      std::vector<int> added_impressioned_clicked_id = find_elements_in_b_not_in_a(previous_impressioned_clicked_id, impressioned_clicked_id);
      recommend_trace_info.added_impressioned_clicked_id = arrayToString(added_impressioned_clicked_id);
      recommend_trace_info.previous_rank_time = previous_recommend_trace_info_value.rank_time;
    }
    else
    {
      recommend_trace_info.previous_rank_time = -1; // previous_rank_time -1 means no previous rank time
    }

    web::json::value params = knowledgebase::convertTerminusRecommendParamsToJsonValue(recommendParams);
    recommend_trace_info.recommend_parameter_json_serialized = params.serialize();
    return recommend_trace_info;
  }

  std::vector<int> getIntegerIdFromVecImpression(const std::vector<Impression> &impressions)
  {
    std::vector<int> result;
    for (const auto &impression : impressions)
    {
      result.push_back(int(impression.integer_id));
    }
    return result;
  }

} // namespace rssrank
