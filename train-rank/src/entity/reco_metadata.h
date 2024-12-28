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

static const char RECOMMEND_TRACE_INFO_USER_EMBEDDING_SOURCE_FIELD[] = "source";
static const char RECOMMEND_TRACE_INFO_USER_EMBEDDING_USER_EMBEDDING_FIELD[] = "user_embedding";
static const char RECOMMEND_TRACE_INFO_USER_EMBEDDING_IMPRESSION_ID_USED_TO_CALCULATE_EMBEDDING_FIELD[] = "impression_id_used_to_calculate_embedding";
static const char RECOMMEND_TRACE_INFO_USER_EMBEDDING_UNIQUE_ID_FIELD[] = "unique_id";
static const char RECOMMEND_TRACE_INFO_USER_EMBEDDING_CREATED_RANK_TIME_FIELD[] = "created_rank_time";

struct RecommendTraceUserEmbedding
{
    string source;
    vector<double> user_embedding;
    std::string impression_id_used_to_calculate_embedding; // 1-3;6;8-9
    string unique_id;                                      // according user_embedding 和 impression_id_used_to_calculate_embedding calculate hash
    long long created_rank_time;
    void setImpressionIdUsedToCalculateEmbedding(const vector<int> &impression_id_used_to_calculate_embedding_vec)
    {
        this->impression_id_used_to_calculate_embedding = arrayToString(impression_id_used_to_calculate_embedding_vec);
    }

    vector<int> getImpressionIdUsedToCalculateEmbeddingVec()
    {
        return stringToArray(this->impression_id_used_to_calculate_embedding);
    }

    void calculateUniqueId()
    {
        this->unique_id = generateSHA256Hash(this->user_embedding, this->impression_id_used_to_calculate_embedding);
    }
    /**
    web::json::value toJson() const
    {
        web::json::value result = web::json::value::object();
        result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_SOURCE_FIELD] = web::json::value::string(source);
        result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_USER_EMBEDDING_FIELD] = web::json::value::array();
        for (const auto &val : user_embedding)
        {
            result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_USER_EMBEDDING_FIELD].as_array().push_back(web::json::value::number(val));
        }
        result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_IMPRESSION_ID_USED_TO_CALCULATE_EMBEDDING_FIELD] = web::json::value::string(impression_id_used_to_calculate_embedding);
        result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_UNIQUE_ID_FIELD] = web::json::value::string(unique_id);
        return result;
    }
    */

    /**
 static RecommendTraceUserEmbedding fromJson(const web::json::value &json)
 {
     RecommendTraceUserEmbedding result;
     result.source = json.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_SOURCE_FIELD).as_string();
     for (const auto &val : json.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_USER_EMBEDDING_FIELD).as_array())
     {
         result.user_embedding.push_back(val.as_double());
     }
     result.impression_id_used_to_calculate_embedding = json.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_IMPRESSION_ID_USED_TO_CALCULATE_EMBEDDING_FIELD).as_string();
     result.unique_id = json.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_UNIQUE_ID_FIELD).as_string();
     return result;
 }
 */
};
//

static const char RECOMMEND_TRACE_INFO_SOURCE_FIELD[] = "source";
static const char RECOMMEND_TRACE_INFO_RANK_TIME_FIELD[] = "rank_time";
static const char RECOMMEND_TRACE_INFO_SCORE_ENUM_FIELD[] = "score_enum";
static const char RECOMMEND_TRACE_INFO_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD[] = "not_impressioned_algorithm_id";
static const char RECOMMEND_TRACE_INFO_ADDED_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD[] = "added_not_impressioned_algorithm_id";
static const char RECOMMEND_TRACE_INFO_IMPRESSIONED_CLICKED_ID_FIELD[] = "impressioned_clicked_id";
static const char RECOMMEND_TRACE_INFO_ADDED_IMPRESSION_CLICKED_ID_FIELD[] = "added_impressioned_clicked_id";
static const char RECOMMEND_TRACE_INFO_LONG_TERM_USER_EMBEDDING_ID_FIELD[] = "long_term_user_embedding_id";
static const char RECOMMEND_TRACE_INFO_SHORT_TERM_USER_EMBEDDING_ID_FIELD[] = "short_term_user_embedding_id";
static const char RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_ID_FIELD[] = "top_ranked_algorithm_id";
static const char RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_SCORE_FIELD[] = "top_ranked_algorithm_score";
static const char RECOMMEND_TRACE_INFO_PREVIOUS_RANK_TIME_FIELD[] = "previous_rank_time";
static const char RECOMMEND_TRACE_INFO_RECALL_USER_EMBEDDING_ID_FIELD[] = "recall_user_embedding_id";
struct RecommendTraceInfo
{
    string source;
    long long rank_time; // unique id
    long long previous_rank_time;
    std::string score_enum;
    std::string not_impressioned_algorithm_id;       // 1-3;6;8-9
    std::string added_not_impressioned_algorithm_id; // The difference between the current not_impressioned_algorithm_id and the last rank time's not_impressioned_algorithm_id, which is the newly added algorithm
    std::string impressioned_clicked_id;             // All impressions at this moment
    std::string added_impressioned_clicked_id;       // Newly added impression_id
    std::string long_term_user_embedding_id;         // RecommendTraceUserEmbedding.unique_id
    std::string short_term_user_embedding_id;        // RecommendTraceUserEmbedding.unique_id
    std::string recall_user_embedding_id;
    vector<int> top_ranked_algorithm_id;      // The top 1000 algorithm_ids, the number of top rankings can be controlled by parameters
    vector<float> top_ranked_algorithm_score; // The top 1000 algorithm_scores, the number of top rankings can be controlled by parameters

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

    void setImpressionedIdFromVec(const vector<int> &impressioned_clicked_id_vec)
    {
        this->impressioned_clicked_id = arrayToString(impressioned_clicked_id_vec);
    }

    vector<int> getImpressionedIdVec()
    {
        return stringToArray(this->impressioned_clicked_id);
    }

    void setAddedImpressionedIdFromVec(const vector<int> &added_impressioned_clicked_id_vec)
    {
        this->added_impressioned_clicked_id = arrayToString(added_impressioned_clicked_id_vec);
    }

    vector<int> getAddedImpressionedIdVec()
    {
        return stringToArray(this->added_impressioned_clicked_id);
    }
    /**
    web::json::value toJson() const
    {
        web::json::value result = web::json::value::object();
        result[RECOMMEND_TRACE_INFO_SOURCE_FIELD] = web::json::value::string(source);
        result[RECOMMEND_TRACE_INFO_RANK_TIME_FIELD] = web::json::value::number(rank_time);
        result[RECOMMEND_TRACE_INFO_SCORE_ENUM_FIELD] = web::json::value::string(score_enum);
        result[RECOMMEND_TRACE_INFO_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD] = web::json::value::string(not_impressioned_algorithm_id);
        result[RECOMMEND_TRACE_INFO_ADDED_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD] = web::json::value::string(added_not_impressioned_algorithm_id);
        result[RECOMMEND_TRACE_INFO_IMPRESSIONED_CLICKED_ID_FIELD] = web::json::value::string(impressioned_clicked_id);
        result[RECOMMEND_TRACE_INFO_ADDED_IMPRESSION_CLICKED_ID_FIELD] = web::json::value::string(added_impressioned_clicked_id);
        result[RECOMMEND_TRACE_INFO_LONG_TERM_USER_EMBEDDING_ID_FIELD] = web::json::value::string(long_term_user_embedding_id);
        result[RECOMMEND_TRACE_INFO_SHORT_TERM_USER_EMBEDDING_ID_FIELD] = web::json::value::string(short_term_user_embedding_id);
        result[RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_ID_FIELD] = web::json::value::array();
        for (const auto &val : top_ranked_algorithm_id)
        {
            result[RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_ID_FIELD].as_array().push_back(web::json::value::number(val));
        }
        result[RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_SCORE_FIELD] = web::json::value::array();
        for (const auto &val : top_ranked_algorithm_score)
        {
            result[RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_SCORE_FIELD].as_array().push_back(web::json::value::number(val));
        }
        return result;
    }

    static RecommendTraceInfo fromJson(const web::json::value &json)
    {
        RecommendTraceInfo result;
        result.source = json.at(RECOMMEND_TRACE_INFO_SOURCE_FIELD).as_string();
        result.rank_time = json.at(RECOMMEND_TRACE_INFO_RANK_TIME_FIELD).as_integer();
        result.score_enum = json.at(RECOMMEND_TRACE_INFO_SCORE_ENUM_FIELD).as_string();
        result.not_impressioned_algorithm_id = json.at(RECOMMEND_TRACE_INFO_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD).as_string();
        result.added_not_impressioned_algorithm_id = json.at(RECOMMEND_TRACE_INFO_ADDED_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD).as_string();
        result.impressioned_clicked_id = json.at(RECOMMEND_TRACE_INFO_IMPRESSIONED_CLICKED_ID_FIELD).as_string();
        result.added_impressioned_clicked_id = json.at(RECOMMEND_TRACE_INFO_ADDED_IMPRESSION_CLICKED_ID_FIELD).as_string();
        result.long_term_user_embedding_id = json.at(RECOMMEND_TRACE_INFO_LONG_TERM_USER_EMBEDDING_ID_FIELD).as_string();
        result.short_term_user_embedding_id = json.at(RECOMMEND_TRACE_INFO_SHORT_TERM_USER_EMBEDDING_ID_FIELD).as_string();
        for (const auto &val : json.at(RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_ID_FIELD).as_array())
        {
            result.top_ranked_algorithm_id.push_back(val.as_integer());
        }
        for (const auto &val : json.at(RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_SCORE_FIELD).as_array())
        {
            result.top_ranked_algorithm_score.push_back(val.as_double());
        }
        return result;
    }
    */
};
ostream &operator<<(ostream &os, const RecommendTraceUserEmbedding &obj);
std::ostream &operator<<(std::ostream &os, const RecommendTraceInfo &info);
#endif // RECO_METADATA_H