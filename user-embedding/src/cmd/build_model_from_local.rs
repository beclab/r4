use userembedding::{
    common,
    embedding_common::{self, MODEL_RELATED_INFO_MAP},
};

fn download_models() -> Result<(), Box<dyn std::error::Error>> {
    for (_, model_related_info) in MODEL_RELATED_INFO_MAP.iter() {
        tracing::info!("Downloading model {}...", model_related_info.model_name);
        let (_, _) = embedding_common::build_model_and_tokenizer_from_local(model_related_info)
            .map_err(|e| {
                tracing::error!("Error building model and tokenizer: {}", e);
                e
            })?;
        tracing::info!(
            "Model {} downloaded successfully",
            model_related_info.model_name
        );
    }

    Ok(())
}

fn main() {
    // Call the function to download models
    common::init_tracing();

    if let Err(e) = download_models() {
        tracing::error!("Error downloading models: {}", e);
    }
}
