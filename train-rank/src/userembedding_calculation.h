#pragma once
#include "entity/impression.h"
std::vector<double> calcluateUserLongTermEmbedding(const std::vector<Impression> &impressions);
std::vector<double> calcluateUserShortTermEmbedding(const std::vector<Impression> &impressions, bool with_weight);
std::vector<float> calcluateUserEmbeddingSimple(const std::vector<std::vector<float>> &embeddings);