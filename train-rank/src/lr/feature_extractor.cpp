#include "feature_extractor.h"

#include <cmath>
#include <vector>

using std::vector;

namespace lr {

namespace {

double embeddingCosine(const vector<double>& emb1, const vector<double>& emb2) {
    double result = -1.0;
    for (int i = 0; i < emb1.size(); ++i) {
        result += emb1[i] * emb2[i];
    }

    return result;
}

}  // namespace

EmbeddingDistanceExtractor::EmbeddingDistanceExtractor() {
}

void EmbeddingDistanceExtractor::addSample(const Impression& imp) {
    if (!imp.embedding.has_value()) {
        return;
    }

    if (imp.clicked) {
        posItems.emplace_back(imp.entry_id, imp.embedding.value());
    } else {
        negItems.emplace_back(imp.entry_id, imp.embedding.value());
    }
}

bool EmbeddingDistanceExtractor::ready() {
    // TODO(haochengwang): Optimize me
    return posItems.size() >= 1;
}

double EmbeddingDistanceExtractor::extract(const Impression& item) {
    if (!item.embedding.has_value()) {
        return 0.0;
    }
    return extract(Item(item.entry_id, item.embedding.value()));
}

double EmbeddingDistanceExtractor::extract(const Algorithm& item) {
    if (!item.embedding.has_value()) {
        return 0.0;
    }
    return extract(Item(item.entry, item.embedding.value()));
}

double EmbeddingDistanceExtractor::extract(const Item& item) {
    double minPosCos = -1.0;
    for (const auto& pos : posItems) {
        minPosCos = std::max(minPosCos, embeddingCosine(pos.embedding, item.embedding));
    }
    double minNegCos = -1.0;
    for (const auto& neg : negItems) {
        minNegCos = std::max(minNegCos, embeddingCosine(neg.embedding, item.embedding));
    }
    // TODO(haochengwang): Optimize me
    return minPosCos + 1.0;
}

Reason EmbeddingDistanceExtractor::getReason(const Algorithm& item) {
    if (!item.embedding.has_value()) {
        return {""};
    }
    double minCos = -1.0;
    const Item* result = nullptr;
    for (const auto& pos : posItems) {
        double cos = embeddingCosine(pos.embedding, item.embedding.value());
        if (cos > minCos) {
            minCos = cos;
            result = &pos;
        }
    }

    if (result == nullptr) {
        return {""};
    }
    return { result->id };
}

}  // namespace lr