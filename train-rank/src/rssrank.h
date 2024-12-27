#pragma once

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace fs = std::filesystem;

#include <xgboost/c_api.h>

#include <gflags/gflags.h>

#include "entity/impression.h"
#include "entity/reco_metadata.h"

DECLARE_string(recommend_source_name);

namespace rssrank
{
    bool trainOneBigBatch();
    bool rankLR();
    bool doRank();
    bool trainLR();
    static const float clicked_weight = 0.1;
    static const float read_finish_weight = 0.2;
    static const float read_speed_weight = 0.3;
    static const float star_weight = 0.4;

    static const float short_term_user_embedding_clicked_weight = 0.1;
    static const float short_term_user_embedding_read_finish_weight = 0.3;
    static const float short_term_user_embedding_stared_weight = 0.4;
    static const float short_term_user_embedding_time_weight = 0.2;

    enum class ModelPathType
    {
        TRAINING,
        PRODUCTION,
    };

    bool getCurrentBatchFromPositiveNegativeSampleWithSpeicifiedSize(
        const std::unordered_set<std::string> &train_positive_sample,
        const std::unordered_set<std::string> &train_negative_sample,
        std::vector<std::string> *current_batch, int positive_batch_size,
        int negative_batch_size);

    const int data_iter_batch_size = 128;
    const int data_iter_positive_batch_size = 32;
    const int data_iter_negative_batch_size = 96;

    std::string getRankModelPath(ModelPathType model_path_type);
    int getCurrentEmbeddingDimension();

    std::unordered_map<std::string, float> inferPredictVector(
        const std::vector<float> &test_features, int test_rows,
        BoosterHandle h_booster, const std::vector<std::string> &id_list);
    void rankPredict();

    void getEachImpressionScoreKnowledge(
        std::unordered_map<std::string, float> *positive,
        std::unordered_map<std::string, float> *negative);
    float getSpecificImpressionScore(const Impression &current_impression);
    enum FillFeatureSource
    {
        ALGORITHM,
        IMPRESSION
    };

    bool fillFeaturesKnowledgeVector(std::vector<std::vector<float>> &features,
                                     const std::vector<std::string> &ids,
                                     enum FillFeatureSource embedding_source);

    std::unordered_map<std::string, std::string> getNotRankedAlgorithmToEntry();
    std::unordered_map<std::string, ScoreWithMetadata>
    getAllEntryToPrerankSourceForCurrentSourceKnowledge();

    bool trainOneBigBatchWithPreparedDataVector(
        const std::vector<float> &features, float *train_labels, const int rows,
        const std::unordered_map<std::string, std::string> &parameters,
        const std::vector<float> &test_features, int test_rows, float *test_labels,
        float *biggest_auc);

    bool trainOneBigBatchPredictVector(const std::vector<float> &test_features,
                                       int test_rows, float *test_labels,
                                       BoosterHandle h_booster, float *biggest_auc);
    std::vector<Impression> getImpressionForShortTermAndLongTermUserEmbeddingRank();
    std::vector<double> calcluateUserShortTermEmbedding(const std::vector<Impression> &impressions, bool with_weight);
    double getTimeCoefficientForUnixTimestamp(long long timestamp, long long current_timestamp);
    float getSpecificImpressionScoreForShortTermUserEmbedding(const Impression &current_impression);
    bool rankShortTermAndLongTermUserEmbedding();
    std::unordered_map<std::string, std::string> getNotImpressionedAlgorithmToEntry();
    vector<double> calcluateUserLongTermEmbedding(const vector<Impression> &impressions);

    bool rankByTimeForColdStart(long long current_time);

} // namespace rssrank
