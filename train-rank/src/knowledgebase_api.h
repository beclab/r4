#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
using std::cerr;
using std::endl;

#include <cpprest/http_client.h>
using namespace utility;
using namespace web::http;
using namespace web::http::client;

#include "entity/entry.h"
#include "entity/impression.h"
#include "entity/rank_algorithm.h"
#include "entity/reco_metadata.h"

namespace knowledgebase {

class EntryCache {
public:
    static EntryCache& getInstance();
    std::optional<Entry> getEntryById(const std::string& id);

    void dumpStatistics();

private:
    EntryCache();
    std::unordered_map<std::string, Entry> cache;
    int cache_miss;
    int cache_hit;
};

static const char ALGORITHM_API_SUFFIX[] = "/knowledge/algorithm";
static const char IMPRESSION_API_SUFFIX[] = "/knowledge/impression";
static const char ENTRY_API_SUFFIX[] = "/knowledge/entry";
static const char CONFIG_API_SUFFIX[] = "/knowledge/config";
static const char LAST_RANK_TIME[] = "last_rank_time";
static const char LAST_EXTRACTOR_TIME[] = "last_extractor_time";

bool updateAlgorithmScoreAndRanked(const std::string& entry_id,
                                   float rank_score, bool ranked);
bool updateAlgorithmScoreAndMetadata(
    const std::unordered_map<std::string, ScoreWithMetadata>& algorithm_id_to_score_with_meta);
bool rerank(const std::string& source);
void getImpression(int limit, int offset, std::string source,
                   std::vector<Impression>* impression_list, int* count);
std::optional<Impression> convertFromWebJsonValueToImpression(
    web::json::value current_item);
std::optional<Algorithm> convertFromWebJsonValueToAlgorithm(
    web::json::value current_item);
std::optional<Entry> convertFromWebJsonValueToEntry(
    web::json::value current_item);
void getAllImpression(std::string source,
                      std::vector<Impression>* impression_list, int* count);
std::optional<Impression> GetImpressionById(const std::string& id);
void getAlgorithmAccordingRanked(int limit, int offset, std::string source,
                                 bool ranked,
                                 std::vector<Algorithm>* algorithm_list,
                                 int* count);
void getAllAlgorithmAccordingRanked(std::string source,
                                    std::vector<Algorithm>* algorithm_list,
                                    bool ranked, int* count);
std::optional<Algorithm> GetAlgorithmById(const std::string& id);
std::optional<Entry> GetEntryById(const std::string& id);
bool updateKnowledgeConfig(const std::string& source, const std::string& key,
                           const web::json::value& value);
bool updateLastRankTime(std::string source, int64_t last_rank_time);
bool updateLastExtractorTime(std::string source, int64_t last_extractor_time);
void convertStringTimestampToInt64(std::string str_timestamp,
                                   int64_t* int64_timestamp);
int64_t getLastRankTime(const std::string& source);
int64_t getLastExtractorTime(const std::string& source);
}  // namespace knowledgebase
