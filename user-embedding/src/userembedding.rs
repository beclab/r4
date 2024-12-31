use log::debug as logdebug;
use userembedding::{bertcommon, common, embedding_common::MODEL_RELATED_INFO_MAP};

#[tokio::main(flavor = "multi_thread", worker_threads = 4)]
async fn main() {
    // common::init_logger();
    /**
    common::init_tracing();
    std::env::var("TERMINUS_RECOMMEND_SOURCE_NAME")
        .expect("TERMINUS_RECOMMEND_SOURCE_NAME not exist");
    let embedding_method: String =
        std::env::var("EMBEDDING_METHOD").expect("EMBEDDING_METHOD not exist");
    std::env::var("KNOWLEDGE_BASE_API_URL").expect("KNOWLEDGE_BASE_API_URL not exist");
    if MODEL_RELATED_INFO_MAP.contains_key(embedding_method.as_str()) == false {
        tracing::error!("embedding method {} not exist", embedding_method);
        return;
    }
    bertcommon::execute_bertv2_user_embedding().await;
    tracing::debug!("execute bertv2_user_embedding compelete");
    */
    common::init_tracing();
    logdebug!("execute bertv2_user_embedding current need not to execute");
}
