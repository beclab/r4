#pragma once

#include <vector>

#include "../entity/rank_algorithm.h"
#include "../entity/impression.h"

namespace lr {

struct Reason {
    std::string reason;
    Reason(const std::string& reason):reason(reason) {}
};

class FeatureExtractor {
public:
    virtual void addSample(const Impression& imp) = 0;
    virtual bool ready() = 0;
    virtual double extract(const Impression& item) = 0;
    virtual double extract(const Algorithm& item) = 0;
    virtual Reason getReason(const Algorithm& item) = 0;
};

class EmbeddingDistanceExtractor : public FeatureExtractor {
public:
    struct Item {
        std::string id;
        std::vector<double> embedding;
        Item(std::string id, const std::vector<double>& embedding): id(id), embedding(embedding) {}
    };
    EmbeddingDistanceExtractor();

    virtual void addSample(const Impression& imp);
    virtual bool ready();
    virtual double extract(const Impression& imp);
    virtual double extract(const Algorithm& item);
    virtual Reason getReason(const Algorithm& item);
private:
    double extract(const Item& item);

    std::vector<Item> posItems;
    std::vector<Item> negItems;
};


} // namespace lr