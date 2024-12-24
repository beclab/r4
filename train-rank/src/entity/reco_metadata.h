#pragma once
#ifndef RECO_METADATA_H
#define RECO_METADATA_H

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
using namespace std;

#include "../common_tool.h"

struct RecoReasonArticle
{
    std::string id;
    std::string title;
    std::string url;

    RecoReasonArticle() {}
    RecoReasonArticle(const std::string &id, const std::string &title, const std::string &url)
        : id(id), title(title), url(url) {}
};

struct RecoReason
{
    std::vector<RecoReasonArticle> articles;
};

enum ScoreEnum
{
    SCORE_PUBLISHED_AT_TIME = 0,
    SCORE_DISTANCE_ARTICLE_WITH_SHORT_AND_LONG_TERM_USER_EMBEDDING_PLUS_PUBLISHED_TIME = 1,
    SCORE_LOGISTIC_REGRESSION_AND_CREATED_TIME = 2,
    SCORE_PRERANK_SCORE_AND_CREATED_TIME = 3,
    SCORE_UNKNOWN = 4

};

extern std::unordered_map<ScoreEnum, std::string> scoreEnumToString;

struct ScoreWithMetadata
{
    double score;
    bool rankExecuted;
    RecoReason reason;
    long long score_rank_time;
    int score_rank_sequence;
    std::string score_rank_method;

    ScoreWithMetadata() {}
    ScoreWithMetadata(double score, long long score_rank_time, ScoreEnum score_rank_enum) : score(score), rankExecuted(false), score_rank_time(score_rank_time)
    {
        if (scoreEnumToString.find(score_rank_enum) != scoreEnumToString.end())
        {
            score_rank_method = scoreEnumToString[score_rank_enum];
        }
        else
        {
            score_rank_method = scoreEnumToString[ScoreEnum::SCORE_UNKNOWN];
        }
    }
};

std::ostream &operator<<(std::ostream &os, const ScoreWithMetadata &score_with_meta);

struct RecommendTraceUserEmbedding
{
    string source;
    vector<double> user_embedding;
    std::string impression_id_used_to_calculate_embedding; // 1-3;6;8-9
    string unique_id;                                      // according user_embedding 和 impression_id_used_to_calculate_embedding calculate hash
    void calculateUniqueId()
    {
        this->unique_id = generateSHA256Hash(this->user_embedding, this->impression_id_used_to_calculate_embedding);
    }
};
//
struct RecommendTraceInfo
{
    string source;
    long long rank_time; // unique id
    ScoreEnum score_enum;
    std::string not_impressioned_algorithm_id;       // 1-3;6;8-9
    std::string added_not_impressioned_algorithm_id; // The difference between the current not_impressioned_algorithm_id and the last rank time's not_impressioned_algorithm_id, which is the newly added algorithm
    std::string impressioned_id;                     // All impressions at this moment
    std::string added_impressioned_id;               // Newly added impression_id
    std::string long_term_user_embedding_id;         // RecommendTraceUserEmbedding.unique_id
    std::string short_term_user_embedding_id;        // RecommendTraceUserEmbedding.unique_id
    vector<int> top_ranked_algorithm_id;             // The top 1000 algorithm_ids, the number of top rankings can be controlled by parameters
    vector<int> top_ranked_algorithm_score;          // The top 1000 algorithm_scores, the number of top rankings can be controlled by parameters

    void setNotImpressionedAlgorithmIdFromVec(const vector<int> &not_impressioned_algorithm_id_vec)
    {
        this->not_impressioned_algorithm_id = arrayToString(not_impressioned_algorithm_id_vec);
    }
    vector<int> getNotImpressionedAlgorithmIdVec()
    {
        return stringToArray(this->not_impressioned_algorithm_id);
    }

    void setAddedNotImpressionedAlgorithmIdFromVec(const vector<int> &added_not_impressioned_algorithm_id_vec)
    {
        this->added_not_impressioned_algorithm_id = arrayToString(added_not_impressioned_algorithm_id_vec);
    }
    vector<int> getAddedNotImpressionedAlgorithmIdVec()
    {
        return stringToArray(this->added_not_impressioned_algorithm_id);
    }

    void setImpressionedIdFromVec(const vector<int> &impressioned_id_vec)
    {
        this->impressioned_id = arrayToString(impressioned_id_vec);
    }

    vector<int> getImpressionedIdVec()
    {
        return stringToArray(this->impressioned_id);
    }

    void setAddedImpressionedIdFromVec(const vector<int> &added_impressioned_id_vec)
    {
        this->added_impressioned_id = arrayToString(added_impressioned_id_vec);
    }

    vector<int> getAddedImpressionedIdVec()
    {
        return stringToArray(this->added_impressioned_id);
    }
};
#endif // RECO_METADATA_H