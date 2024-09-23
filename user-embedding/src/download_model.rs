use hf_hub::api::sync::Api;

use userembedding::{
    common,
    embedding_common::{self, MODEL_RELATED_INFO_MAP},
};

fn download_models() -> Result<(), Box<dyn std::error::Error>> {
    for (_, model_related_info) in MODEL_RELATED_INFO_MAP.iter() {
        tracing::info!("Downloading model {}...", model_related_info.model_name);
        let default_model: String = model_related_info.hugging_face_model_name.to_string();
        let default_revision: String = model_related_info.hugging_face_model_revision.to_string();
        let (_, _) =
            embedding_common::build_model_and_tokenizer(default_model, default_revision).unwrap();
        tracing::info!(
            "Model {} downloaded successfully",
            model_related_info.model_name
        );
    }

    Ok(())
}

fn main() {
    // Call the function to download models
    common::init_logger();

    if let Err(e) = download_models() {
        eprintln!("Error downloading models: {}", e);
    }
}
