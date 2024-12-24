#include "reco_metadata.h"

std::ostream &operator<<(std::ostream &os, const ScoreWithMetadata &score_with_meta)
{
    os << score_with_meta.score << "|";
    if (score_with_meta.rankExecuted)
    {
        os << "RANKED|";
    }
    else
    {
        os << "UNRANKED|";
    }
    for (const auto &article : score_with_meta.reason.articles)
    {
        os << "[" << article.id << "," << article.title << "," << article.url << "]";
    }
    return os;
}

std::unordered_map<ScoreEnum, std::string>
    scoreEnumToString = {
        {SCORE_PUBLISHED_AT_TIME, "SCORE_PUBLISHED_At_TIME"},
        {SCORE_DISTANCE_ARTICLE_WITH_SHORT_AND_LONG_TERM_USER_EMBEDDING_PLUS_PUBLISHED_TIME, "SCORE_DISTANCE_ARTICLE_WITH_SHORT_AND_LONG_TERM_USER_EMBEDDING_PLUS_PUBLISHED_TIME"},
        {SCORE_LOGISTIC_REGRESSION_AND_CREATED_TIME, "SCORE_LOGISTIC_REGRESSION_AND_CREATED_TIME"},
        {SCORE_PRERANK_SCORE_AND_CREATED_TIME, "SCORE_PRERANK_SCORE_AND_CREATED_TIME"},
        {SCORE_UNKNOWN, "SCORE_UNKNOWN"}};
