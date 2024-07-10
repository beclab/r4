#include <iostream>
#include <map>
#include <string>
#include <gflags/gflags.h>

#include "../rssrank.h"
#include "../common_tool.h"
#include "../knowledgebase_api.h"

using std::endl;
using std::pair;
using std::unordered_map;
using std::vector;
using std::string;

namespace {

double embeddingDist(const vector<double>& emb1, const vector<double>& emb2) {
    double result = .0;
    for (int i = 0; i < emb1.size(); ++i) {
        result += emb1[i] * emb2[i];
    }

    return result;
}

}  // namespace

DEFINE_string(distance_base, "",
    "Entry ID of the entry which would be treated as the baseline of the distance calculation");
DEFINE_int32(count, 10, "The size of the result.");

void setupLogging() {
   el::Configurations defaultConf;
   defaultConf.setToDefault();
    // Values are always std::string
   defaultConf.set(el::Level::Debug,
            el::ConfigurationType::Enabled, "false");
    // default logger uses default configurations
   el::Loggers::reconfigureLogger("default", defaultConf);
}

int main(int argc, char** argv) {
    setupLogging();
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_recommend_source_name.size() == 0) {
        LOG(ERROR) << "recommend_source_name not set" << endl;
        return -1;
    }

    vector<Algorithm> ranked, unranked;
    int ranked_count, unranked_count;
    knowledgebase::getAllAlgorithmAccordingRanked(
        FLAGS_recommend_source_name, &ranked, true, &ranked_count);
    knowledgebase::getAllAlgorithmAccordingRanked(
        FLAGS_recommend_source_name, &unranked, false, &unranked_count);

    LOG(INFO) << "Ranked: " << ranked_count << " items, Actual " << ranked.size() << " items" << endl;
    LOG(INFO) << "Unanked: " << unranked_count << " items, Actual " << unranked.size() << " items" << endl;

    // Check embedding exist
    unordered_map<string, Algorithm*> idmap;
    int noembedding_count = 0;
    for (auto& item : ranked) {
        if (!item.embedding.has_value()) {
            ++noembedding_count;
        }
        idmap[item.entry] = &item;
    }
    for (auto& item : unranked) {
        if (!item.embedding.has_value()) {
            ++noembedding_count;
        }
        idmap[item.entry] = &item;
    }
    LOG(INFO) << noembedding_count << " out of " << idmap.size() << " entries have no embedding." << endl;

    int count = 10;
    for (auto& pr : idmap) {
        auto entry = pr.first;
        auto title = knowledgebase::GetEntryById(entry).value().title;
        LOG(INFO) << "Sample: " << entry << ", title = " << title << endl;
        if (--count <= 0)
            break;
    }

    if (FLAGS_distance_base.size() == 0) {
        LOG(INFO) << "No distance_base specified, quit." << endl;
        return 0;
    }
    if (idmap.find(FLAGS_distance_base) == idmap.end()) {
        LOG(ERROR) << "No entry with id = " << FLAGS_distance_base << " found, quit." << endl;
        return -1;
    }
    auto base_embedding = idmap[FLAGS_distance_base]->embedding.value();

    vector<pair<double, string>> distances;
    for (auto& pr : idmap) {
        if (pr.first == FLAGS_distance_base) {
            continue;
        }
        distances.emplace_back(embeddingDist(base_embedding, pr.second->embedding.value()), pr.first);
    }


    sort(distances.rbegin(), distances.rend());
    count = FLAGS_count;
    int i = 1;
    for (auto& pr : distances) {
        auto entry = pr.second;
        auto title = knowledgebase::GetEntryById(entry).value().title;
        LOG(INFO) << i++ << ": Cosine = " << pr.first << ", id = " << entry << ", title = " << title << endl;
        if (--count <= 0)
            break;
    }

    return 0;
}