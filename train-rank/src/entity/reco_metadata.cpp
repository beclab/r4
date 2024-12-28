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

ostream &operator<<(ostream &os, const RecommendTraceUserEmbedding &obj)
{

    os << "Source: " << obj.source << "\n"
       << "User Embedding Size: " << obj.user_embedding.size() << "\n"
       << "Impression ID Used to Calculate Embedding: " << obj.impression_id_used_to_calculate_embedding << "\n"
       << "created_rank_time: " << obj.created_rank_time << "\n"
       << "Unique ID: " << obj.unique_id;
    return os;
}

std::ostream &operator<<(std::ostream &os, const RecommendTraceInfo &info)
{
    os << "source: " << info.source << "\n";
    os << "rank_time: " << info.rank_time << "\n";
    os << "previous_rank_time: " << info.previous_rank_time << "\n";
    os << "score_enum: " << info.score_enum << "\n";
    os << "not_impressioned_algorithm_id: " << info.not_impressioned_algorithm_id << "\n";
    os << "added_not_impressioned_algorithm_id: " << info.added_not_impressioned_algorithm_id << "\n";
    os << "impressioned_clicked_id: " << info.impressioned_clicked_id << "\n";
    os << "added_impressioned_clicked_id: " << info.added_impressioned_clicked_id << "\n";
    os << "long_term_user_embedding_id: " << info.long_term_user_embedding_id << "\n";
    os << "short_term_user_embedding_id: " << info.short_term_user_embedding_id << "\n";
    os << "top_ranked_algorithm_id size: " << info.top_ranked_algorithm_id.size() << "\n";
    os << "top_ranked_algorithm_score size: " << info.top_ranked_algorithm_score.size() << "\n";

    return os;
}