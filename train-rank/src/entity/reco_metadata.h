#pragma once

#include <iostream>
#include <string>
#include <vector>

struct RecoReasonArticle {
    std::string id;
    std::string title;
    std::string url;

    RecoReasonArticle() {}
    RecoReasonArticle(const std::string& id, const std::string& title, const std::string& url)
        :id(id), title(title), url(url) {}
};

struct RecoReason {
    std::vector<RecoReasonArticle> articles;
};

struct ScoreWithMetadata {
    double score;
    bool rankExecuted;
    RecoReason reason;

    ScoreWithMetadata() {}
    ScoreWithMetadata(double score): score(score), rankExecuted(false) {}
};

std::ostream& operator<<(std::ostream& os, const ScoreWithMetadata& score_with_meta);