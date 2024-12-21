#pragma once
#ifndef RECO_METADATA_H
#define RECO_METADATA_H

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
using namespace std;

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
#endif // RECO_METADATA_H