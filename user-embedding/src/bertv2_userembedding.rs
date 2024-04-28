use log::debug as logdebug;
use userembedding::{bertv2, common};

#[tokio::main(flavor = "multi_thread", worker_threads = 4)]
async fn main() {
    common::init_logger();
    std::env::var("TERMINUS_RECOMMEND_SOURCE_NAME")
        .expect("TERMINUS_RECOMMEND_SOURCE_NAME not exist");
    std::env::var("KNOWLEDGE_BASE_API_URL").expect("KNOWLEDGE_BASE_API_URL not exist");
    bertv2::execute_bertv2_user_embedding().await;
    logdebug!("execute bertv2_user_embedding compelete");
}
