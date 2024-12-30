#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <gflags/gflags.h>

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

DECLARE_string(recommend_source_name);

namespace knowledgebase
{

    class EntryCache
    {
    public:
        static EntryCache &getInstance();
        void init();
        std::optional<Entry> getEntryById(const std::string &id);
        std::optional<Entry> getEntryByIntegerId(int integer_id);
        void loadAllEntries();

        void dumpStatistics();
        long long getMinLastOpened() { return min_last_opened; }
        long long getMaxLastOpened() { return max_last_opened; }

    private:
        EntryCache();
        std::unordered_map<std::string, Entry> cache;
        std::unordered_map<int, std::string> cache_integer_id_to_id;
        int cache_miss;
        int cache_hit;
        long long min_last_opened;
        long long max_last_opened;
    };

    static const char ALGORITHM_API_SUFFIX[] = "/knowledge/algorithm";
    static const char IMPRESSION_API_SUFFIX[] = "/knowledge/impression";
    static const char ENTRY_API_SUFFIX[] = "/knowledge/entry";
    static const char CONFIG_API_SUFFIX[] = "/knowledge/config";
    static const char RECOMMEND_TRACE_USER_EMBEDDING_API_SUFFIX[] = "/knowledge/userembedding";
    static const char RECOMMEND_TRACE_INFO_API_SUFFIX[] = "/knowledge/recommendtraceinfo";

    static const char LAST_RANK_TIME[] = "last_rank_time";
    static const char LAST_EXTRACTOR_TIME[] = "last_extractor_time";
    // static const char LONG_TERM_USER_EMBEDDING[] = "user_embedding";
    static const char REDIS_KEY_RECALL_USER_EMBEDDING[] = "user_embedding";

    bool updateAlgorithmScoreAndRanked(const std::string &entry_id,
                                       float rank_score, bool ranked);
    bool updateAlgorithmScoreAndMetadata(
        const std::unordered_map<std::string, ScoreWithMetadata> &algorithm_id_to_score_with_meta);
    bool rerank(const std::string &source);
    void getImpression(int limit, int offset, std::string source,
                       std::vector<Impression> *impression_list, int *count);
    std::optional<Impression> convertFromWebJsonValueToImpression(
        web::json::value current_item);
    std::optional<Algorithm> convertFromWebJsonValueToAlgorithm(
        web::json::value current_item);
    std::optional<Entry> convertFromWebJsonValueToEntry(
        web::json::value current_item);
    void getAllImpression(std::string source,
                          std::vector<Impression> *impression_list, int *count);
    std::optional<Impression> GetImpressionById(const std::string &id);
    std::optional<Impression> GetImpressionByIntegerId(int integer_id);
    bool postRecommendTraceUserEmbedding(const RecommendTraceUserEmbedding &embedding);
    bool postRecommendTraceInfo(const RecommendTraceInfo &info);
    void getAlgorithmAccordingRanked(int limit, int offset, std::string source,
                                     bool ranked,
                                     std::vector<Algorithm> *algorithm_list,
                                     int *count);
    void getAllAlgorithmAccordingRanked(std::string source,
                                        std::vector<Algorithm> *algorithm_list,
                                        bool ranked, int *count);
    void getAlgorithmAccordingImpression(int limit, int offset, std::string source,
                                         int impression,
                                         std::vector<Algorithm> *algorithm_list,
                                         int *count);
    std::optional<Algorithm> GetAlgorithmById(const std::string &id);
    std::optional<Algorithm> GetAlgorithmByIntegerId(int integer_id);
    std::optional<Entry> GetEntryById(const std::string &id);
    std::unordered_map<std::string, Entry> getEntries(const std::string &source);
    std::unordered_map<std::string, Entry> getEntries(const std::string &source);
    void getEntries(int limit, int offset, const std::string &source,
                    std::vector<Entry> *entry_list, int *count);
    bool updateKnowledgeConfig(const std::string &source, const std::string &key,
                               const web::json::value &value);
    bool updateLastRankTime(std::string source, int64_t last_rank_time);
    bool updateRecallUserEmbedding(std::string source,
                                   const std::vector<double> &long_term_user_embedding, long long current_time);
    bool updateLastExtractorTime(std::string source, int64_t last_extractor_time);
    void convertStringTimestampToInt64(std::string str_timestamp,
                                       int64_t *int64_timestamp);
    int64_t getLastRankTime(const std::string &source);
    int64_t getLastExtractorTime(const std::string &source);
    std::vector<double> getRecallUserEmbedding(const std::string &source);

    std::vector<double> parse_embedding(const std::string &input, size_t embedding_dimension);
    std::vector<double> init_user_embedding(size_t embedding_dimension);
    std::vector<std::pair<std::string, float>> rankScoreMetadata(const std::unordered_map<std::string, ScoreWithMetadata> &algorithm_id_to_score_with_meta);
    bool updateAlgorithmScoreAndMetadataWithScoreOrder(
        const std::unordered_map<std::string, ScoreWithMetadata> &algorithm_id_to_score_with_meta);
    std::optional<web::json::value> convertFromRecommendTraceUserEmbeddingToWebJsonValue(
        const RecommendTraceUserEmbedding &embedding);
    std::optional<web::json::value> convertFromRecommendTraceInfoToWebJsonValue(
        const RecommendTraceInfo &info);
    std::optional<RecommendTraceUserEmbedding> convertFromWebJsonValueToRecommendTraceUserEmbedding(
        const web::json::value &value);
    std::optional<RecommendTraceUserEmbedding> findRecommendTraceUserEmbeddingByUniqueId(const std::string &unique_id);

    std::optional<RecommendTraceInfo> convertFromWebJsonValueToRecommendTraceInfo(
        const web::json::value &value);
    std::optional<RecommendTraceInfo> findRecommendTraceInfoByRankTimeAndSource(const std::string &source, int64_t rank_time);
    std::vector<int> findAllRecomendTraceInfoRankTimesBySource(const std::string &source);
    std::optional<std::string> getKnowledgeCofnig(const string &source, const string &key);
    void init_global_terminus_recommend_params();
    web::json::value convertTerminusRecommendParamsToJsonValue(const TerminusRecommendParams &params);

} // namespace knowledgebase
