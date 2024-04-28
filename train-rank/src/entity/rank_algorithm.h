#pragma once

#include <string>
#include <vector>
#include <optional>

static const char ALGORITHM_MONGO_FIELD_ID[] = "_id";
static const char ALGORITHM_MONGO_FIELD_ENTRY[] = "entry";
static const char ALGORITHM_MONGO_FIELD_SOURCE[] = "source";
static const char ALGORITHM_MONGO_FIELD_SCORE[] = "score";
static const char ALGORITHM_MONGO_FIELD_IMPRESSION[] = "impression";
static const char ALGORITHM_MONGO_FIELD_RANKED[] = "ranked";
static const char ALGORITHM_MONGO_FIELD_EXTRA[] = "extra";

static const char ALGORITHM_MONGO_FIELD_EMBEDDING[] =
    "embedding";  // SUB FIELD ALGORITHM_MONGO_FIELD_EXTRA
static const char ALGORITHM_MONGO_FIELD_PRERANK_SCORE[] =
    "prerank_score";  // SUB FIELD ALGORITHM_MONGO_FIELD_EXTRA

struct Algorithm {
  std::string id;
  std::string entry;
  std::string source;
  bool ranked;
  double score;
  int impression;
  std::optional<std::vector<double>> embedding;
  std::optional<double> prerank_score;
};
