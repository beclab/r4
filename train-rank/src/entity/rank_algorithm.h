#pragma once

#include <string>
#include <vector>
#include <optional>

static const char ALGORITHM_MONGO_FIELD_ID[] = "id";
static const char ALGORITHM_MONGO_FIELD_ENTRY[] = "entry";
static const char ALGORITHM_MONGO_FIELD_SOURCE[] = "source";
static const char ALGORITHM_MONGO_FIELD_SCORE[] = "score";
static const char ALGORITHM_MONGO_FIELD_IMPRESSION[] = "impression";
static const char ALGORITHM_MONGO_FIELD_RANKED[] = "ranked";
static const char ALGORITHM_MONGO_FIELD_EXTRA[] = "extra";
static const char ALGORITHM_MONGO_FIELD_SCORE_RANK_TIME[] = "score_rank_time";
static const char ALGORITHM_MONGO_FIELD_SCORE_RANK_METHOD[] = "score_rank_method";
static const char ALGORITHM_MONGO_FIELD_SCORE_RANK_SEQUENCE[] = "score_rank_sequence";

static const char ALGORITHM_MONGO_FIELD_EMBEDDING[] =
    "embedding"; // SUB FIELD ALGORITHM_MONGO_FIELD_EXTRA
static const char ALGORITHM_MONGO_FIELD_PRERANK_SCORE[] =
    "prerank_score"; // SUB FIELD ALGORITHM_MONGO_FIELD_EXTRA

struct Algorithm
{
  std::string id;
  std::string entry;
  std::string source;
  bool ranked;
  double score;
  int impression;
  std::optional<std::vector<double>> embedding;
  std::optional<double> prerank_score;
  std::optional<long long> score_rank_time;
  std::optional<std::string> score_rank_method;
  int score_rank_sequence;
};
