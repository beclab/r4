#pragma once

#include <string>
#include <vector>
#include <optional>

static const char IMPRESSION_MONGO_FIELD_STARED[] = "stared";
static const char IMPRESSION_MONGO_FIELD_BATCH_ID[] = "batch_id";
static const char IMPRESSION_MONGO_FIELD_POSITION[] = "position";
static const char IMPRESSION_MONGO_FIELD_READ_TIME[] = "read_time";
static const char IMPRESSION_MONGO_FIELD_READ_FINISH[] = "read_finish";
static const char IMPRESSION_MONGO_FIELD_ID[] = "id";
static const char IMPRESSION_MONGO_FIELD_ENTRY_ID[] = "entry_id";
static const char IMPRESSION_MONGO_FIELD_CLICKED[] = "clicked";
static const char IMPRESSION_MONGO_FIELD_ALGORITHM_EXTRA[] = "algorithm_extra";
static const char IMPRESSION_MONGO_FIELD_SOURCE[] = "source";

// SUB FIELD IMPRESSION_MONGO_FIELD_ALGORITHM_EXTRA
static const char IMPRESSION_MONGO_FIELD_EMBEDDING[] = "embedding";

struct Impression {
  bool stared;
  std::string batch_id;
  std::string position;
  float read_time;
  bool read_finish;
  std::string id;
  std::string entry_id;
  bool clicked;
  std::optional<std::vector<double>> embedding;
  std::string source;
};
