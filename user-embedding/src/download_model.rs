use std::{fs::File, io::Write};

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
        let (_, _, current_bert_model_file_path) =
            embedding_common::build_model_and_tokenizer_from_internet(
                default_model,
                default_revision,
            )
            .unwrap();
        let serialized = serde_json::to_string(&current_bert_model_file_path).map_err(|e| {
            log::error!("Error serializing model file path: {}", e);
            e
        })?;

        let output_json_path = format!(
            "/root/.cache/huggingface/{}.json",
            model_related_info.model_name
        );

        let mut file = File::create(output_json_path).map_err(|e| {
            log::error!("Error creating file: {}", e);
            e
        })?;
        file.write_all(serialized.as_bytes()).map_err(|e| {
            log::error!("Error writing to file: {}", e);
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
    common::init_logger();

    if let Err(e) = download_models() {
        tracing::error!("Error downloading models: {}", e);
    }
}
