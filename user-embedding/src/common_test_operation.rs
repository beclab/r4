use crate::common;
use std::env;
pub fn init_env() {
    common::init_logger();
    env::set_var("TERMINUS_RECOMMEND_MONGODB_URI",  "mongodb://root:example@localhost:27017/?authSource=admin&readPreference=primary&ssl=false&directConnection=true");
    env::set_var("TERMINUS_RECOMMEND_MONGODB_NAME", "document");
    env::set_var("TERMINUS_RECOMMEND_SOURCE_NAME", "bert_v2");
    env::set_var("TERMINUS_RECOMMEND_REDIS_ADDR", "127.0.0.1:6381");
    env::set_var(
        "TERMINUS_RECOMMEND_REDIS_PASSOWRD",
        "terminusrecommendredis123",
    );
    env::set_var("KNOWLEDGE_BASE_API_URL", "http://52.202.37.138:3010")
    // env::set_var("TERMINUS_RECOMMEND_REDIS_URI", "redis://127.0.0.1:6381");
    // env::set_var("TERMINUS_RECOMMEND_REDIS_URI", "redis://:terminusrecommendredis123@127.0.0.1:6381");
}
