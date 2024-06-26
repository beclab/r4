#include <stdlib.h>
void initDevelop() {
  /**
  setenv("TERMINUS_RECOMMEND_MONGODB_URI",
         "mongodb://root:example@localhost:27017/"
         "?authSource=admin&readPreference=primary&ssl=false&directConnection="
         "true",
         0);
  */
  setenv(TERMINUS_RECOMMEND_SOURCE_NAME, "r4world", 0);
  setenv(TERMINUS_RECOMMEND_EMBEDDING_NAME, "bert_v2", 0);
  setenv(KNOWLEDGE_BASE_API_URL, "http://52.202.37.138:3010", 0);
  setenv(TERMINUS_RECOMMEND_EMBEDDING_DIMENSION, "384", 0);
}