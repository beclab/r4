use std::{collections::HashMap, fs, ops::Add};

use anyhow::{bail, Error as AnyhowError, Result as AnyhowResult};
use candle_core::{DType, Device, Result as CandleResult, Tensor};
use candle_nn::VarBuilder;
use candle_transformers::models::bert::{BertModel, Config, DTYPE};
use hf_hub::{api::sync::Api, Repo, RepoType};
use log::{debug as logdebug, error as logerror};
use ndarray::Array;
use ndarray_rand::rand_distr::Uniform;
use ndarray_rand::RandomExt;
use serde::{Deserialize, Serialize};
use text_splitter::TextSplitter;
use tokenizers::Tokenizer;

use crate::{
    common,
    entity::impression::Impression,
    knowledge_base_api::{self, get_impression_pagination},
};

pub fn build_device(cpu: bool) -> CandleResult<Device> {
    if cpu {
        Ok(Device::Cpu)
    } else {
        let device = Device::cuda_if_available(0)?;
        if !device.is_cuda() {
            println!("Running on CPU, to run on GPU, build this example with `--features cuda`");
        }
        Ok(device)
    }
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct BertModelFilePath {
    pub config_filename: String,
    pub tokenizer_filename: String,
    pub weights_filename: String,
}

pub fn build_model_and_tokenizer_from_local(
    current_model_info_field: &ModelInfoField,
) -> AnyhowResult<(BertModel, Tokenizer)> {
    let device = build_device(false).map_err(|e| {
        tracing::error!("build device error {}", e);
        AnyhowError::msg(e)
    })?;
    let current_model_file_path = format!(
        "/userembedding/model/{}.json",
        current_model_info_field.model_name
    );
    tracing::info!(
        "current_model_file_path {}",
        current_model_file_path.clone()
    );
    let file_content = fs::read_to_string(current_model_file_path.clone()).map_err(|e| {
        tracing::error!(
            "read file error {} current_model_file_path {}",
            e,
            current_model_file_path
        );
        AnyhowError::msg(e)
    })?;
    tracing::info!("file_content {}", file_content);
    let data: BertModelFilePath = serde_json::from_str(&file_content).map_err(|e| {
        tracing::error!("parse json error {}", e);
        AnyhowError::msg(e)
    })?;
    let config_filename = fs::canonicalize(data.config_filename.clone()).map_err(|e| {
        tracing::error!(
            "canonicalize error {} config_filename {}",
            e,
            data.config_filename.clone()
        );
        AnyhowError::msg(e)
    })?;
    let tokenizer_filename = fs::canonicalize(data.tokenizer_filename.clone()).map_err(|e| {
        tracing::error!(
            "canonicalize error {} tokenizer_filename {}",
            e,
            data.tokenizer_filename.clone()
        );
        AnyhowError::msg(e)
    })?;
    let weights_filename = fs::canonicalize(data.weights_filename.clone()).map_err(|e| {
        tracing::error!(
            "canonicalize error {} weights_filename {}",
            e,
            data.weights_filename.clone()
        );
        AnyhowError::msg(e)
    })?;
    let config = std::fs::read_to_string(config_filename.clone()).map_err(|e| {
        tracing::error!("read config file error {} {}", e, config_filename.display());
        AnyhowError::msg(e)
    })?;
    let config: Config = serde_json::from_str(&config).map(|config| {
        tracing::info!("config {:?}", config);
        config
    })?;
    let tokenizer = Tokenizer::from_file(tokenizer_filename.clone()).map_err(|e| {
        tracing::error!(
            "tokenizer from file error {} {}",
            e,
            tokenizer_filename.display()
        );
        AnyhowError::msg(e)
    })?;
    let vb = unsafe {
        VarBuilder::from_mmaped_safetensors(&[weights_filename.clone()], DTYPE, &device).map_err(
            |e| {
                tracing::error!(
                    "VarBuilder from mmaped safetensors error {} {}",
                    e,
                    weights_filename.display()
                );
                AnyhowError::msg(e)
            },
        )
    }?;
    let model = BertModel::load(vb, &config).map_err(|e| {
        tracing::error!("load model error {}", e);
        AnyhowError::msg(e)
    })?;
    tracing::info!("compelete build model and tokenizer from local");
    Ok((model, tokenizer))
}

pub fn build_model_and_tokenizer_from_internet(
    default_model: String,
    default_revision: String,
) -> AnyhowResult<(BertModel, Tokenizer, BertModelFilePath)> {
    let device = build_device(false)?;

    let repo = Repo::with_revision(default_model, RepoType::Model, default_revision);
    let (config_filename, tokenizer_filename, weights_filename) = {
        let api = Api::new()?;
        let api = api.repo(repo);
        let config = api.get("config.json")?;
        let tokenizer = api.get("tokenizer.json")?;
        let weights = api.get("model.safetensors")?;

        (config, tokenizer, weights)
    };
    let current_bert_model_file_path = BertModelFilePath {
        config_filename: config_filename.display().to_string(),
        tokenizer_filename: tokenizer_filename.display().to_string(),
        weights_filename: weights_filename.display().to_string(),
    };
    tracing::debug!(
        "[{}] [{}]  config_filename {} tokenizer_filename {} weights_filename {}",
        file!(),
        line!(),
        config_filename.display(),
        tokenizer_filename.display(),
        weights_filename.display()
    );

    let config = std::fs::read_to_string(config_filename)?;
    let config: Config = serde_json::from_str(&config)?;
    let tokenizer = Tokenizer::from_file(tokenizer_filename).map_err(AnyhowError::msg)?;
    let vb = unsafe { VarBuilder::from_mmaped_safetensors(&[weights_filename], DTYPE, &device)? };
    let model = BertModel::load(vb, &config)?;
    Ok((model, tokenizer, current_bert_model_file_path))
}

pub fn normalize_l2(v: &Tensor, dimension: usize) -> AnyhowResult<Tensor> {
    let sum = v
        .sum_all()
        .expect("get result error")
        .to_vec0::<f32>()
        .expect("convert fail");
    let whether_eq = float_cmp::approx_eq!(f32, 0f32, sum, ulps = 2);
    if whether_eq {
        Ok(v.clone())
    } else {
        Ok(v.broadcast_div(&v.sqr()?.sum_keepdim(dimension)?.sqrt()?)?)
    }
}

pub type TokenizerImplSimple = tokenizers::TokenizerImpl<
    tokenizers::ModelWrapper,
    tokenizers::NormalizerWrapper,
    tokenizers::PreTokenizerWrapper,
    tokenizers::PostProcessorWrapper,
    tokenizers::DecoderWrapper,
>;

pub fn parse_user_embedding(user_embedding: &String, embedding_dimension: usize) -> (Tensor, bool) {
    let mut split_result: Vec<&str> = user_embedding.split(";").collect();
    let mut empty_embedding_data: Vec<f32> = Vec::with_capacity(embedding_dimension);
    // let mut cumulative_tensor = Tensor::new(&cumulative_embedding_data, &Device::Cpu)?;
    let mut parse_success = true;
    if split_result.len() == embedding_dimension + 1 {
        split_result.pop();
        for current_element in split_result {
            let convert_result = current_element.parse::<f32>();
            match convert_result {
                Ok(current_float) => {
                    empty_embedding_data.push(current_float);
                }
                Err(err) => {
                    tracing::error!("convert user embedding fail {}", err.to_string());
                    parse_success = false;
                    break;
                }
            }
        }
    }

    if empty_embedding_data.len() != embedding_dimension {
        parse_success = false;
        empty_embedding_data.clear();
        let mut index = 0;
        loop {
            if index == embedding_dimension {
                break;
            }
            empty_embedding_data.push(0f32);
            index = index + 1;
        }
    }

    let embedding_tensor: Tensor =
        Tensor::new(empty_embedding_data.as_slice(), &Device::Cpu).expect("msg");
    return (embedding_tensor, parse_success);
}

pub fn calculate_one_sentence(
    model: &BertModel,
    tokenizer: &TokenizerImplSimple,
    current_sentence: String,
    max_characters: usize,
) -> AnyhowResult<Tensor, AnyhowError> {
    if current_sentence.trim().len() == 0 {
        bail!(common::SentenceEmptyError {})
    }

    let splitter: TextSplitter<text_splitter::Characters> = TextSplitter::default()
        // Optionally can also have the splitter trim whitespace for you
        .with_trim_chunks(true);
    let chunks: Vec<&str> = splitter
        .chunks(current_sentence.as_str(), max_characters)
        .collect();

    let device: &Device = &model.device;

    let mut last_tensor: Tensor = Tensor::zeros((1, 384), candle_core::DType::F32, device)?;
    let mut count: i32 = 0;
    for current_chunk in chunks {
        let tokens: Vec<u32> = tokenizer
            .encode(current_chunk, false)
            .map_err(AnyhowError::msg)?
            .get_ids()
            .to_vec();
        let token_ids: Tensor = Tensor::new(&tokens[..], device)?.unsqueeze(0)?;
        let token_type_ids: Tensor = token_ids.zeros_like()?;
        let ys: Tensor = model.forward(&token_ids, &token_type_ids)?;
        let (_n_sentence, n_tokens, _hidden_size) = ys.dims3()?;
        let embeddings: Tensor = (ys.sum(1)? / (n_tokens as f64))?;
        let normalized_embeddings: Tensor = normalize_l2(&embeddings, 1)?;
        last_tensor = last_tensor.add(normalized_embeddings).unwrap();
        count = count + 1;
        if count >= 3 {
            break;
        }
    }

    Ok(last_tensor)
}

pub fn init_user_embedding(embedding_dimension: usize) -> Tensor {
    let random_data3: Vec<f32> = Array::random(embedding_dimension, Uniform::new(0., 1.)).to_vec();
    let mut random_data_array = Vec::with_capacity(embedding_dimension);
    let mut index = 0;
    for current_item in random_data3 {
        random_data_array.push(current_item);
        index = index + 1
    }
    let embedding_tensor: Tensor = Tensor::new(random_data_array.as_slice(), &Device::Cpu)
        .expect("create init user embedding tensor fail");
    return embedding_tensor;
}

pub async fn set_user_embedding_knowledgebase(user_embedding: &Tensor) {
    let vec_user_embedding: Vec<f32> = user_embedding
        .to_vec1::<f32>()
        .expect("convert user embedding to vec fail");
    let str_user_embedding: String = common::join_array(&vec_user_embedding);
    let current_time = common::get_time_now().to_string();
    let user_embedding_time = format!("{}{}{}", str_user_embedding, ";", current_time);
    knowledge_base_api::set_user_embedding_str_through_knowledge(
        user_embedding_time,
        vec_user_embedding.len(),
    )
    .await;
}

pub async fn retrieve_user_embedding_through_knowledge(
    embedding_dimension: usize,
) -> AnyhowResult<Tensor, AnyhowError> {
    let get_user_embedding_str_result = knowledge_base_api::get_user_embedding_str().await;
    let parse_result = parse_user_embedding(&get_user_embedding_str_result.0, embedding_dimension);
    return Ok(parse_result.0);
}

pub async fn retrieve_wise_library_impression_knowledge(
) -> AnyhowResult<HashMap<String, String>, AnyhowError> {
    let mut impression_id_to_entry_id: HashMap<String, String> = HashMap::<String, String>::new(); // impression_id_to_entry_id whose embedding not exist

    let batch_size = 100;
    let mut offset = 0;
    let mut count: i64 = 0;
    let wise = String::from("wise");
    let library = String::from("library");
    loop {
        let mut temp_impression_list: Vec<Impression> = Vec::new();
        get_impression_pagination(
            batch_size,
            offset,
            None,
            &mut temp_impression_list,
            &mut count,
            &wise,
        )
        .await;
        tracing::debug!(
            "offset {} limit {} count {} current_batch_size {} wise",
            offset,
            batch_size,
            count,
            temp_impression_list.len()
        );
        for current_impression in temp_impression_list {
            impression_id_to_entry_id.insert(current_impression.id, current_impression.entry_id);
        }
        offset = offset + batch_size;
        if offset > batch_size {
            break;
        }
    }

    offset = 0;
    loop {
        let mut temp_impression_list: Vec<Impression> = Vec::new();
        get_impression_pagination(
            batch_size,
            offset,
            Some(true),
            &mut temp_impression_list,
            &mut count,
            &library,
        )
        .await;
        tracing::debug!(
            "offset {} limit {} count {} current_batch_size {} library",
            offset,
            batch_size,
            count,
            temp_impression_list.len()
        );
        for current_impression in temp_impression_list {
            impression_id_to_entry_id.insert(current_impression.id, current_impression.entry_id);
        }
        offset = offset + batch_size;
        if offset > batch_size {
            break;
        }
    }

    Ok(impression_id_to_entry_id)
}

pub async fn retrieve_current_algorithm_impression_knowledge(
    source_name: String,
    embedding_dimension: usize,
) -> Option<Tensor> {
    let mut cumulative_tensor = Tensor::zeros(embedding_dimension, DType::F32, &Device::Cpu)
        .expect("create zeros cumulative tensor fail");
    let batch_size = 100;
    let mut offset = 0;
    let mut count = 0;
    loop {
        let mut temp_impression_list: Vec<Impression> = Vec::new();
        get_impression_pagination(
            batch_size,
            offset,
            Some(true),
            &mut temp_impression_list,
            &mut count,
            &source_name,
        )
        .await;
        tracing::debug!(
            "offset {} limit {} count {} current_batch_size {}",
            offset,
            batch_size,
            count,
            temp_impression_list.len()
        );
        for current_impression in temp_impression_list {
            tracing::debug!(
                "current impression id {} entry id {}",
                current_impression.id,
                current_impression.entry_id
            );
            if let Some(current_embedding) = current_impression.embedding {
                if current_embedding.len() != embedding_dimension {
                    tracing::error!(
                        "current entry {} embedding dimension not equal {}",
                        current_embedding.len(),
                        embedding_dimension
                    );
                    continue;
                }
                let current_impression_embedding_tensor =
                    Tensor::new(current_embedding, &Device::Cpu)
                        .expect("create tensor from embedding fail");
                cumulative_tensor = cumulative_tensor
                    .add(current_impression_embedding_tensor)
                    .expect("add tensor fail");
            }
            tracing::debug!("add impression {} embedding success", current_impression.id);
        }
        offset = offset + batch_size;
        if offset > batch_size {
            break;
        }
    }
    return Some(cumulative_tensor);
}

pub struct ModelInfoField {
    pub model_name: &'static str,
    pub hugging_face_model_name: &'static str,
    pub hugging_face_model_revision: &'static str,
    pub embedding_dimension: usize,
}

lazy_static! {
    pub static ref MODEL_RELATED_INFO_MAP: HashMap<&'static str, ModelInfoField> = {
        let mut m = HashMap::new();
        m.insert(
            "bert_v2",
            ModelInfoField {
                model_name: "bert_v2",
                hugging_face_model_name: "sentence-transformers/all-MiniLM-L6-v2",
                hugging_face_model_revision: "refs/pr/21",
                embedding_dimension: 384,
            },
        );
        m.insert(
            "bert_v3",
            ModelInfoField {
                model_name: "bert_v3",
                hugging_face_model_name:
                    "sentence-transformers/paraphrase-multilingual-MiniLM-L12-v2",
                hugging_face_model_revision: "refs/heads/main",
                embedding_dimension: 384,
            },
        );
        m
    };
}

#[cfg(test)]
mod embeddingcommontest {

    use anyhow::Error as AnyhowError;

    use crate::{bertcommon, common_test_operation, embedding_common};

    use super::calculate_one_sentence;
    use super::*;

    #[tokio::test]
    async fn test_retrieve_current_algorithm_impression_knowledge() {
        // cargo test embeddingcommontest::test_retrieve_current_algorithm_impression_knowledge
        common_test_operation::init_env();
        let source_name = String::from("bert_v2");

        let current_tensor = retrieve_current_algorithm_impression_knowledge(source_name, 384)
            .await
            .expect("add cumulative tensor fail");
        tracing::error!("current_tensor {}", current_tensor);
    }

    #[tokio::test]
    async fn test_retrieve_wise_library_impression_knowledge() {
        // cargo test embeddingcommontest::test_retrieve_wise_library_impression_knowledge
        common_test_operation::init_env();
        let result = retrieve_wise_library_impression_knowledge()
            .await
            .expect("retrieve error");
        tracing::debug!("size  {}", result.len());
    }

    #[test]
    fn test_sum() {
        let current_tensor: Tensor =
            Tensor::new(&[2f32, 3f32], &Device::Cpu).expect("new tensor fail");
        let sum_tensor = current_tensor
            .sum_all()
            .expect("get result error")
            .to_vec0::<f32>()
            .expect("convert fail");
        let normailzed1 = normalize_l2(&current_tensor, 0).expect("normalize failed");
        println!("11111111111111{}", normailzed1);
        println!("11111111111111111111{}", sum_tensor);
        let current_tensor2: Tensor =
            Tensor::new(&[0f32, 0f32], &Device::Cpu).expect("new tensor fail");
        let normalized2 = normalize_l2(&current_tensor2, 0).expect("normalize error");
        println!("222222222222{}", normalized2);
    }

    #[test]
    fn test_parse_user_embedding() {
        let first_string = String::from("1.1;1.2;ok;3333");
        let parse_result = parse_user_embedding(&first_string, 3);
        println!("***{}  {}", parse_result.0, parse_result.1)
    }

    #[test]
    fn test_build_model_and_tokenizer_from_internet() {
        // env::set_var("CUDA_COMPUTE_CAP","86");
        // cargo test embeddingcommontest::test_build_model_and_tokenizer_from_internet
        common::init_logger();
        let default_model = "sentence-transformers/all-MiniLM-L6-v2".to_string();
        let default_revision = "refs/pr/21".to_string();
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
        let current_tensor = calculate_one_sentence(
            &model,
            current_tokenizer,
            String::from("How beautiful girl"),
            512,
        )
        .unwrap();
        println!("*******************result {:?}", current_tensor);
        let result = current_tensor.get(0).unwrap().to_vec1::<f32>().unwrap();
        println!("********************** vec<f32> {:?}", result);
        // let result = current_tensor.to_vec1::<f32>().unwrap();
        // println!("*******************result {:?}",result);
        println!("{}", current_tensor);
        let result = calculate_one_sentence(&model, current_tokenizer, String::from(""), 512);
        match result {
            Ok(tensor) => println!("tensor {:?}", tensor.shape()),
            Err(err) => println!("err {}", err),
        }
    }

    #[test]
    fn test_build_model_and_tokenizer_from_local() {
        // cargo test embeddingcommontest::test_build_model_and_tokenizer_from_local
        common::init_logger();
        let current_model_info_field = embedding_common::MODEL_RELATED_INFO_MAP
            .get("bert_v2")
            .unwrap();
        let (model, mut tokenizer) =
            embedding_common::build_model_and_tokenizer_from_local(current_model_info_field)
                .unwrap();
        let current_tokenizer: &TokenizerImplSimple = tokenizer
            .with_padding(None)
            .with_truncation(None)
            .map_err(AnyhowError::msg)
            .unwrap();
        let current_tensor = calculate_one_sentence(
            &model,
            current_tokenizer,
            String::from("How beautiful the blonde girl"),
            500,
        )
        .unwrap();
        println!("*******************result {:?}", current_tensor);
        let result = current_tensor.get(0).unwrap().to_vec1::<f32>().unwrap();
        println!("********************** vec<f32> {:?}", result);
    }
}
