use std::{collections::HashMap, env};

use anyhow::{Error as AnyhowError, Result as AnyhowResult};
use candle_core::{Device, Tensor};
use candle_transformers::models::bert::BertModel;
use log::{debug as logdebug, error as logerror};
use ndarray_rand::rand;
use rand::seq::SliceRandom;
use tokenizers::Tokenizer;

use crate::{
    embedding_common::{self, normalize_l2, TokenizerImplSimple},
    knowledge_base_api,
};

// const HUGGING_FACE_MODEL_NAME: &str = "sentence-transformers/all-MiniLM-L6-v2";
// const HUGGING_FACE_MODEL_REVISION: &str = "refs/pr/21";
pub const BERT_V2_EMBEDDING_DIMENSION: usize = 384;
pub const BERT_V3_EMBEDDING_DIMENSION: usize = 384;

pub async fn calculate_single_entry_pure(
    current_entry_id: &String,
    model: &BertModel,
    current_tokenizer: &embedding_common::TokenizerImplSimple,
) -> Option<Tensor> {
    let current_entry_option = knowledge_base_api::get_entry_by_id(current_entry_id).await;
    if let Some(current_entry) = current_entry_option {
        if let Some(current_title) = current_entry.titile {
            let current_result: Result<Tensor, AnyhowError> =
                embedding_common::calculate_one_sentence(
                    &model,
                    current_tokenizer,
                    current_title,
                    512,
                );
            match current_result {
                Ok(current_tensor) => {
                    let last_tensor_result = current_tensor.get(0);
                    match last_tensor_result {
                        Ok(current_last_tensor) => {
                            return Some(current_last_tensor);
                        }
                        Err(err) => {
                            logerror!(
                                "get entry {} embedding zero dimension error {}",
                                current_entry_id,
                                err.to_string()
                            );

                            return None;
                        }
                    }
                }
                Err(err) => {
                    logerror!(
                        "calculate entry {} embedding error {}",
                        current_entry_id,
                        err.to_string()
                    );
                    return None;
                }
            }
        } else {
            logerror!("entry {} have not title", current_entry_id);
            return None;
        }
    } else {
        logerror!("entry {} not exist", current_entry_id);
        return None;
    }
}

async fn calculate_userembedding() -> AnyhowResult<Tensor, AnyhowError> {
    let current_source_name: String = env::var("TERMINUS_RECOMMEND_SOURCE_NAME")
        .expect("TERMINUS_RECOMMEND_SOURCE_NAME env not found.");
    let embedding_method: String =
        std::env::var("EMBEDDING_METHOD").expect("EMBEDDING_METHOD not exist");
    let model_related_info: &embedding_common::ModelInfoField =
        embedding_common::MODEL_RELATED_INFO_MAP
            .get(embedding_method.as_str())
            .unwrap();
    let mut option_cumulative_tensor: Option<Tensor> = None;
    if model_related_info.model_name == "bert_v2" {
        let cumulative_embedding_data: [f32; BERT_V2_EMBEDDING_DIMENSION] =
            [0f32; BERT_V2_EMBEDDING_DIMENSION];
        option_cumulative_tensor = Some(Tensor::new(&cumulative_embedding_data, &Device::Cpu)?);
    } else if model_related_info.model_name == "bert_v3" {
        let cumulative_embedding_data: [f32; BERT_V3_EMBEDDING_DIMENSION] =
            [0f32; BERT_V3_EMBEDDING_DIMENSION];
        option_cumulative_tensor = Some(Tensor::new(&cumulative_embedding_data, &Device::Cpu)?);
    } else {
        tracing::error!("embedding method {} not exist", embedding_method);
        return Err(AnyhowError::msg("embedding method not exist"));
    }
    let mut cumulative_tensor: Tensor = option_cumulative_tensor.unwrap();
    let default_model: String = model_related_info.hugging_face_model_name.to_string();
    let default_revision: String = model_related_info.hugging_face_model_revision.to_string();
    let MODEL_INTERNET: String = env::var("MY_ENV_VAR").unwrap_or("false".to_string());
    let mut model_option: Option<BertModel> = None;
    let mut model_tokenizer: Option<Tokenizer> = None;

    if MODEL_INTERNET == "true" {
        logdebug!("use internet model");
        let (model, mut tokenizer, _) = embedding_common::build_model_and_tokenizer_from_internet(
            default_model,
            default_revision,
        )
        .unwrap();
        model_option = Some(model);
        model_tokenizer = Some(tokenizer);
    } else {
        logdebug!("use local model");
        let (model, mut tokenizers) =
            embedding_common::build_model_and_tokenizer_from_local(model_related_info).unwrap();
        model_option = Some(model);
        model_tokenizer = Some(tokenizers);
    }
    let model: BertModel = model_option.unwrap();
    let mut tokenizer: Tokenizer = model_tokenizer.unwrap();
    let current_tokenizer: &TokenizerImplSimple = tokenizer
        .with_padding(None)
        .with_truncation(None)
        .map_err(AnyhowError::msg)
        .unwrap();

    let impression_id_to_entry_id: HashMap<String, String> =
        embedding_common::retrieve_wise_library_impression_knowledge().await?;
    let mut wise_library_entry_ids: Vec<String> = Vec::new();
    for (_, current_entry_id) in impression_id_to_entry_id {
        wise_library_entry_ids.push(current_entry_id)
    }
    let one_hundred_batch_wise_library: Vec<&String> = wise_library_entry_ids
        .choose_multiple(&mut rand::thread_rng(), 100)
        .collect();
    for current_entry_id in one_hundred_batch_wise_library {
        let current_tensor_option =
            calculate_single_entry_pure(current_entry_id, &model, current_tokenizer).await;
        if let Some(current_tensor) = current_tensor_option {
            cumulative_tensor = cumulative_tensor.add(&current_tensor)?;
            logdebug!("add current_entry {}", current_entry_id);
        } else {
            logerror!("current_entry_id {} calculate fail", current_entry_id);
        }
    }

    let current_algorithm_tensor_option =
        embedding_common::retrieve_current_algorithm_impression_knowledge(
            current_source_name.clone(),
            model_related_info.embedding_dimension,
        )
        .await;
    if let Some(current_algorithm_tensor) = current_algorithm_tensor_option {
        logdebug!(
            "current algorithm existing embedding cumulative result {:?}",
            current_algorithm_tensor.to_vec1::<f32>().unwrap()
        );
        cumulative_tensor = cumulative_tensor.add(&current_algorithm_tensor)?;
    } else {
        logerror!(
            "retrieve source {} embedding tensor fail ",
            current_source_name.clone()
        );
    }

    cumulative_tensor = normalize_l2(&cumulative_tensor, 0)?;
    logdebug!(
        "cumulative_tensor {:?}",
        cumulative_tensor.to_vec1::<f32>().unwrap()
    );
    Ok(cumulative_tensor)
}

pub async fn execute_bertv2_user_embedding() {
    let embedding_method: String =
        std::env::var("EMBEDDING_METHOD").expect("EMBEDDING_METHOD not exist");
    let model_related_info: &embedding_common::ModelInfoField =
        embedding_common::MODEL_RELATED_INFO_MAP
            .get(embedding_method.as_str())
            .unwrap();
    let user_embedding: Tensor = calculate_userembedding()
        .await
        .expect("calculate user embedding fail");
    let original_user_embedding = embedding_common::retrieve_user_embedding_through_knowledge(
        model_related_info.embedding_dimension,
    )
    .await
    .expect("retrieve user embedding through knowledge base fail");
    let new_user_embedding_result = user_embedding.add(&original_user_embedding);
    match new_user_embedding_result {
        Ok(current_new_user_embedding) => {
            let normalized_new_user_embedding = normalize_l2(&current_new_user_embedding, 0)
                .expect("normalize new user embedding fail");
            embedding_common::set_user_embedding_knowledgebase(&normalized_new_user_embedding)
                .await;
            logdebug!("set success for new user embedding");
        }
        Err(err) => {
            logerror!("old and new user embedding add fail {},", err.to_string())
        }
    }
}

mod bertv2test {
    use std::env;
    // Note this useful idiom: importing names from outer (for mod tests) scope.
    use super::*;
    use crate::common;
    use crate::common_test_operation;

    #[tokio::test]
    async fn test_calculate_single_entry() {
        // cargo test bertv2test::test_calculate_single_entry
        common_test_operation::init_env();
        let embedding_method: String =
            std::env::var("EMBEDDING_METHOD").expect("EMBEDDING_METHOD not exist");
        let model_related_info: &embedding_common::ModelInfoField =
            embedding_common::MODEL_RELATED_INFO_MAP
                .get(embedding_method.as_str())
                .unwrap();

        let default_model: String = model_related_info.hugging_face_model_name.to_string();
        let default_revision: String = model_related_info.hugging_face_model_revision.to_string();
        let (model, mut tokenizer, _) = embedding_common::build_model_and_tokenizer_from_internet(
            default_model,
            default_revision,
        )
        .unwrap();
        let current_tokenizer: &TokenizerImplSimple = tokenizer
            .with_padding(None)
            .with_truncation(None)
            .map_err(AnyhowError::msg)
            .unwrap();
        let current_entry_id: String = "6555bef91d141d0bf00224ec".to_string();
        let current_tesnor =
            calculate_single_entry_pure(&current_entry_id, &model, current_tokenizer)
                .await
                .expect("get tensor fail");
        logdebug!(
            "current_tensor***************************** {}",
            current_tesnor
        )
    }

    #[tokio::test]
    async fn test_set_init_user_embedding() {
        common_test_operation::init_env();
        let init_embedding = embedding_common::init_user_embedding(384);
        embedding_common::set_user_embedding_knowledgebase(&init_embedding).await;
    }
}
