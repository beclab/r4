#include <gflags/gflags.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "gitinfo.h"
#include "rssrank.h"
#include "common_tool.h"
#include "dump_traceinfo.h"

int main(int argc, char **argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  init_log();
  LOG(INFO) << "Git rev: " << GIT_REV;
  LOG(INFO) << "Git tag: " << GIT_TAG;
  LOG(INFO) << "Git branch: " << GIT_BRANCH;

  const char *source_name = std::getenv(TERMINUS_RECOMMEND_SOURCE_NAME);
  if (source_name == nullptr)
  {
    LOG(ERROR) << TERMINUS_RECOMMEND_SOURCE_NAME << " NOT EXIST" << std::endl;
    exit(-1);
  }
  LOG(DEBUG) << TERMINUS_RECOMMEND_SOURCE_NAME << source_name << std::endl;

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
  // rssrank::rankPredict();
  //  rssrank::rankLR();
  bool rank_success = rssrank::rankShortTermAndLongTermUserEmbedding();
  LOG(DEBUG) << "compelete rank" << std::endl;
  if (rank_success == true)
  {
    dump_traceinfo_main(std::string(source_name));
  }
}
