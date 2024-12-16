#include <stdlib.h>
void initDevelop()
{
  /**
  setenv("TERMINUS_RECOMMEND_MONGODB_URI",
         "mongodb://root:example@localhost:27017/"
         "?authSource=admin&readPreference=primary&ssl=false&directConnection="
         "true",
         0);
  */
  setenv(TERMINUS_RECOMMEND_EMBEDDING_NAME, "bert_v2", 0);
  setenv(KNOWLEDGE_BASE_API_URL, "http://127.0.0.1:3010", 0);
  setenv(TERMINUS_RECOMMEND_EMBEDDING_DIMENSION, "384", 0);
  setenv(TERMINUS_RECOMMEND_SOURCE_NAME, "r4techbiz", 0);
  /**
   * because glag , we need to set env in the command line before run the test
  export TERMINUS_RECOMMEND_EMBEDDING_NAME="bert_v2"
  export KNOWLEDGE_BASE_API_URL="http://127.0.0.1:3010"
  export TERMINUS_RECOMMEND_EMBEDDING_DIMENSION="384"
  export TERMINUS_RECOMMEND_SOURCE_NAME="r4techbiz"
   */
}