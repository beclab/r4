use std::{
    fmt::format,
    fs::{self, File},
    io::Write,
    path::Path,
};

use candle_transformers::models::bert::BertModel;
use hf_hub::api::sync::Api;

use userembedding::{
    common,
    embedding_common::{self, BertModelFilePath, MODEL_RELATED_INFO_MAP},
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
        let current_target_model_directory =
            format!("/userembedding/model/{}", model_related_info.model_name);
        fs::create_dir_all(current_target_model_directory.clone()).map_err(|e| {
            tracing::error!("Error creating directory: {}", e);
            e
        })?;
        let new_current_bert_model_file_path = BertModelFilePath {
            config_filename: format!(
                "/userembedding/model/{}/{}",
                model_related_info.model_name,
                Path::new(&current_bert_model_file_path.config_filename)
                    .file_name()
                    .unwrap()
                    .to_str()
                    .unwrap()
            ),
            tokenizer_filename: format!(
                "/userembedding/model/{}/{}",
                model_related_info.model_name,
                Path::new(&current_bert_model_file_path.tokenizer_filename)
                    .file_name()
                    .unwrap()
                    .to_str()
                    .unwrap()
            ),
            weights_filename: format!(
                "/userembedding/model/{}/{}",
                model_related_info.model_name,
                Path::new(&current_bert_model_file_path.weights_filename)
                    .file_name()
                    .unwrap()
                    .to_str()
                    .unwrap()
            ),
        };
        fs::copy(
            current_bert_model_file_path.config_filename.clone(),
            new_current_bert_model_file_path.config_filename.clone(),
        )
        .map_err(|e| {
            tracing::error!("Error renaming config file: {}", e);
            e
        })?;
        fs::copy(
            current_bert_model_file_path.tokenizer_filename.clone(),
            new_current_bert_model_file_path.tokenizer_filename.clone(),
        )
        .map_err(|e| {
            tracing::error!("Error renaming tokenizer file: {}", e);
            e
        })?;
        fs::copy(
            current_bert_model_file_path.weights_filename.clone(),
            new_current_bert_model_file_path.weights_filename.clone(),
        )
        .map_err(|e| {
            tracing::error!("Error renaming weights file: {}", e);
            e
        })?;

        let serialized = serde_json::to_string(&new_current_bert_model_file_path).map_err(|e| {
            tracing::error!("Error serializing model file path: {}", e);
            e
        })?;

        let output_json_path = format!(
            "/userembedding/model/{}.json",
            model_related_info.model_name
        );

        let mut file = File::create(output_json_path).map_err(|e| {
            tracing::error!("Error creating file: {}", e);
            e
        })?;
        file.write_all(serialized.as_bytes()).map_err(|e| {
            tracing::error!("Error writing to file: {}", e);
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
