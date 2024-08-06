#include "reco_metadata.h"

std::ostream& operator<<(std::ostream& os, const ScoreWithMetadata& score_with_meta) {
    os << score_with_meta.score << "|";
    if (score_with_meta.rankExecuted) {
        os << "RANKED|";
    } else {
        os << "UNRANKED|";
    }
    for (const auto& article : score_with_meta.reason.articles) {
        os << "[" << article.id << "," << article.title << "," << article.url << "]";
    }
    return os;
}