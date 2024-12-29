#include <gflags/gflags.h>

#include "rssrank.h"
#include "common_tool.h"
#include "knowledgebase_api.h"

int main(int argc, char **argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  init_log();
  const char *source_name = std::getenv(TERMINUS_RECOMMEND_SOURCE_NAME);
  if (source_name == nullptr)
  {
    LOG(ERROR) << TERMINUS_RECOMMEND_SOURCE_NAME << " NOT EXIST" << std::endl;
    exit(-1);
  }
  LOG(DEBUG) << TERMINUS_RECOMMEND_SOURCE_NAME << " " << source_name
             << std::endl;

  const char *knowledge_base_api_uri = std::getenv(KNOWLEDGE_BASE_API_URL);
  if (knowledge_base_api_uri == nullptr)
  {
    LOG(ERROR) << KNOWLEDGE_BASE_API_URL << " NOT EXIST" << std::endl;
    exit(-1);
  }
  LOG(DEBUG) << KNOWLEDGE_BASE_API_URL << " " << knowledge_base_api_uri
             << std::endl;
  setenv(TERMINUS_RECOMMEND_EMBEDDING_NAME, "bert_v2", 0);
  setenv(TERMINUS_RECOMMEND_EMBEDDING_DIMENSION, "384", 0);

  // Since currently the ranking logic is rule-based and not relying on the model, train
  // is not necessary for now, comment it out.
  // rssrank::trainOneBigBatch();
  // rssrank::trainLR();
  // knowledgebase::rerank(std::string(source_name));
  // LOG(DEBUG) << "compelete train and ranked reset" << std::endl;
  // knowledgebase::updateLastRankTime(source_name, 0);
  // LOG(DEBUG) << "compelete set last_rank_time 0" << std::endl;
  LOG(DEBUG) << "for rankShortTermAndLongTermUserEmbedding do nothing" << std::endl;
}