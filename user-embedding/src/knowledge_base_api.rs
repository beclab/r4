use crate::embedding_common;
use crate::entity;
use crate::entity::entry;
use crate::entity::entry::Entry;
use crate::entity::impression;
use crate::entity::impression::Impression;
use log::{debug as logdebug, error as logerror};

use reqwest;

pub const CONFIG_API_SUFFIX: &str = "knowledge/config";
pub const ALGORITHM_API_SUFFIX: &str = "knowledge/algorithm";
pub const IMPRESSION_API_SUFFIX: &str = "knowledge/impression";
pub const ENTRY_API_SUFFIX: &str = "knowledge/entry";

pub fn convert_from_web_json_to_impression(current_item: &serde_json::Value) -> Option<Impression> {
    /*
               Object {
                 "batch_id": String("false"),
                 "position": String("false"),
                 "_id": String("659f9ccc584a4a82eaef3bb9"),
                 "entry_id": String("6555fc001d141d0bf0025595"),
                 "clicked": Bool(true),
                 "read_finish": Bool(true),
                 "stared": Bool(false),
                 "read_time": Number(75),
                 "algorithm_extra": Object {
                     "embedding": Array [
                         Number(-0.06527794897556305),
    */
    let mut current_impression = Impression::new();

    let impression_id_option = current_item.get(impression::MONGO_ID);
    if let Some(current_impresssion_id) = impression_id_option {
        let current_impression_id_option: Option<&str> = current_impresssion_id.as_str();
        if let Some(current_impression_id_str) = current_impression_id_option {
            current_impression.id = String::from(current_impression_id_str);
        } else {
            logerror!("current impression have no id");
            return None;
        }
    } else {
        logdebug!(
            "current impression {} have no batch id ",
            current_impression.id
        );
        return None;
    }

    let entry_id_option: Option<&serde_json::Value> = current_item.get(impression::MONGO_ENTRY_ID);
    if let Some(current_entry_id) = entry_id_option {
        let current_entry_id_option: Option<&str> = current_entry_id.as_str();
        if let Some(current_entry_id_str) = current_entry_id_option {
            current_impression.entry_id = String::from(current_entry_id_str);
        } else {
            logerror!(
                "current_impression {} have no entry id",
                current_impression.id
            );
            return None;
        }
    } else {
        logerror!(
            "current_impression {} have no entry id",
            current_impression.id
        );
        return None;
    }

    let batch_id_option: Option<&serde_json::Value> = current_item.get(impression::MONGO_BATCH_ID);
    if let Some(current_batch_id) = batch_id_option {
        let current_batch_id_option: Option<&str> = current_batch_id.as_str();
        if let Some(current_batch_id_str) = current_batch_id_option {
            current_impression.batch_id = Some(String::from(current_batch_id_str));
        } else {
            current_impression.batch_id = None;
            logdebug!(
                "current impression {} have no batch id ",
                current_impression.id
            )
        }
    } else {
        current_impression.batch_id = None;
        logdebug!(
            "current impression {} have no batch id ",
            current_impression.id
        )
    }

    let position_option = current_item.get(impression::MONGO_POSITION);
    if let Some(current_position) = position_option {
        let current_position_option = current_position.as_str();
        if let Some(current_position_str) = current_position_option {
            current_impression.position = Some(String::from(current_position_str));
        } else {
            logdebug!(
                "current impression {} have no position ",
                current_impression.id
            )
        }
    } else {
        current_impression.position = None;
        logdebug!(
            "current impression {} have no position",
            current_impression.id
        )
    }

    let clicked_option: Option<&serde_json::Value> = current_item.get(impression::MONGO_CLICKED);
    if let Some(current_clicked) = clicked_option {
        let current_clicked_option = current_clicked.as_bool();
        if let Some(current_clicked_bool) = current_clicked_option {
            current_impression.clicked = Some(current_clicked_bool)
        } else {
            current_impression.clicked = None;
            logdebug!(
                "current impression {} have no clicked ",
                current_impression.id
            )
        }
    } else {
        current_impression.clicked = None;
        logdebug!(
            "current impression {} have no clicked ",
            current_impression.id
        )
    }

    let stared_option: Option<&serde_json::Value> = current_item.get(impression::MONGO_STARED);
    if let Some(current_stared) = stared_option {
        let current_stared_option = current_stared.as_bool();
        if let Some(current_stared_bool) = current_stared_option {
            current_impression.stared = Some(current_stared_bool)
        } else {
            current_impression.stared = None;
            logdebug!(
                "current impression {} have no stared ",
                current_impression.id
            )
        }
    } else {
        current_impression.stared = None;
        logdebug!(
            "current impression {} have no stared ",
            current_impression.id
        )
    }

    let read_finish_option: Option<&serde_json::Value> =
        current_item.get(impression::MONGO_READ_FINISH);
    if let Some(current_read_finish) = read_finish_option {
        let current_read_finish_option = current_read_finish.as_bool();
        if let Some(current_read_finish_bool) = current_read_finish_option {
            current_impression.read_finish = Some(current_read_finish_bool)
        } else {
            current_impression.read_finish = None;
            logdebug!(
                "current impression {} have no read_finish ",
                current_impression.id
            )
        }
    } else {
        current_impression.stared = None;
        logdebug!(
            "current impression {} have no read_finish ",
            current_impression.id
        )
    }

    let read_time_option: Option<&serde_json::Value> =
        current_item.get(impression::MONGO_READ_TIME);
    if let Some(current_read_time) = read_time_option {
        let current_read_time_option = current_read_time.as_f64();
        if let Some(current_read_time_f64) = current_read_time_option {
            current_impression.read_time = Some(current_read_time_f64)
        } else {
            current_impression.read_time = None;
            logdebug!(
                "current impression {} have no read_finish ",
                current_impression.id
            )
        }
    } else {
        current_impression.read_time = None;
        logdebug!(
            "current impression {} have no read_finish ",
            current_impression.id
        )
    }

    let algorithm_extra_option = current_item.get(impression::MONGO_ALGORITHM_EXTRA);
    if let Some(current_algorithm_extra) = algorithm_extra_option {
        let embedding_option = current_algorithm_extra.get(impression::MONGO_EMBEDDING);
        if let Some(current_embedding) = embedding_option {
            let current_embedding_array_option = current_embedding.as_array();

            if let Some(current_embedding_array) = current_embedding_array_option {
                let mut current_embedding_vec: Vec<f32> = Vec::new();
                let mut flag = true;
                for current_value in current_embedding_array {
                    let current_float_option: Option<f64> = current_value.as_f64();
                    if let Some(current_float) = current_float_option {
                        let convert_float_value = current_float as f32;
                        current_embedding_vec.push(convert_float_value);
                    } else {
                        flag = false;
                        logdebug!(
                            "current impression {} embedding not float ",
                            current_impression.id
                        );
                        break;
                    }
                }
                if flag {
                    current_impression.embedding = Some(current_embedding_vec);
                }
            } else {
                logdebug!(
                    "current impression {} have no embedding ",
                    current_impression.id
                )
            }
        } else {
            logdebug!(
                "current impression {} have no embedding ",
                current_impression.id
            )
        }
    } else {
        logdebug!(
            "current impression {} have no algorithm extra ",
            current_impression.id
        )
    }

    return Some(current_impression);
}

pub fn convert_from_web_json_to_entry(current_item: &serde_json::Value) -> Option<Entry> {
    let mut current_entry = Entry::new();

    let entry_id_option = current_item.get(entry::MONGO_ID);
    if let Some(current_entry_id) = entry_id_option {
        let current_entry_id_option: Option<&str> = current_entry_id.as_str();
        if let Some(current_entry_id_str) = current_entry_id_option {
            current_entry.id = String::from(current_entry_id_str);
        } else {
            logerror!("current entry have no id");
            return None;
        }
    } else {
        logdebug!("current entry have no  id ");
        return None;
    }

    let feed_id_option = current_item.get(entry::MONGO_FEED_ID);
    if let Some(current_feed_id) = feed_id_option {
        let current_feed_id_option: Option<&str> = current_feed_id.as_str();
        if let Some(current_feed_id_str) = current_feed_id_option {
            current_entry.feed_id = Some(String::from(current_feed_id_str));
        } else {
            logerror!("current entry {} have no feed id", current_entry.id);
        }
    } else {
        logdebug!("current entry {} have no feed id", current_entry.id);
    }

    let title_option = current_item.get(entry::MONGO_TITLE);
    if let Some(current_title) = title_option {
        let current_title_option: Option<&str> = current_title.as_str();
        if let Some(current_title_str) = current_title_option {
            current_entry.titile = Some(String::from(current_title_str));
        } else {
            logerror!("current entry {} have no title", current_entry.id);
        }
    } else {
        logdebug!("current entry {} have no title", current_entry.id);
    }

    let url_option: Option<&serde_json::Value> = current_item.get(entry::MONGO_URL);
    if let Some(current_url) = url_option {
        let current_url_option: Option<&str> = current_url.as_str();
        if let Some(current_url_str) = current_url_option {
            current_entry.url = Some(String::from(current_url_str));
        } else {
            logerror!("current entry {} have no url", current_entry.id);
        }
    } else {
        logdebug!("current entry {} have no url", current_entry.id);
    }

    let full_content_option: Option<&serde_json::Value> =
        current_item.get(entry::MONGO_FULL_CONTENT);
    if let Some(current_full_content) = full_content_option {
        let current_full_content_option: Option<&str> = current_full_content.as_str();
        if let Some(current_full_content_str) = current_full_content_option {
            current_entry.full_content = Some(String::from(current_full_content_str));
        } else {
            logerror!("current entry {} have no full content", current_entry.id);
        }
    } else {
        logdebug!("current entry {} have no full content", current_entry.id);
    }

    let raw_content_option: Option<&serde_json::Value> = current_item.get(entry::MONGO_RAW_CONTENT);
    if let Some(current_raw_content) = raw_content_option {
        let current_raw_content_option: Option<&str> = current_raw_content.as_str();
        if let Some(current_raw_content_str) = current_raw_content_option {
            current_entry.raw_content = Some(String::from(current_raw_content_str));
        } else {
            logerror!("current entry {} have no raw content", current_entry.id);
        }
    } else {
        logdebug!("current entry {} have no raw content", current_entry.id);
    }

    return Some(current_entry);
}

pub async fn get_all_impression(
    clicked: Option<bool>,
    impression_list: &mut Vec<Impression>,
    count: &mut i64,
    source_name: &String,
) {
    let batch_size = 100;
    let mut offset = 0;
    loop {
        let mut temp_impression_list: Vec<Impression> = Vec::new();
        get_impression_pagination(
            batch_size,
            offset,
            clicked,
            &mut temp_impression_list,
            count,
            source_name,
        )
        .await;
        logdebug!(
            "offset {} limit {} count {} current_batch_size {}",
            offset,
            batch_size,
            count,
            temp_impression_list.len()
        );
        for current_impression in temp_impression_list {
            impression_list.push(current_impression);
        }
        offset = offset + batch_size;
        if offset > batch_size {
            break;
        }
    }
}

pub async fn get_entry_by_id(entry_id: &str) -> Option<entity::entry::Entry> {
    /*
    Object {
      "code": Number(0),
      "message": Null,
      "data": Object {
          "algorithms": Array [],
          "sources": Array [],
          "labels": Array [],
          "readlater": Bool(false),
          "crawler": Bool(false),
          "starred": Bool(false),
          "disabled": Bool(false),
          "saved": Bool(false),
          "unread": Bool(false),
          "extract": Bool(false),
          "language": String("false"),
          "_id": String("6555bef91d141d0bf00224ec"),
          "url": String("https://phys.org/news/2023-11-climate-effects-marine-ecosystems-multiple.html"),
     */
    // let current_entry: Entry = Entry::new();
    let knowledge_base_prefix: String =
        std::env::var("KNOWLEDGE_BASE_API_URL").expect("KNOWLEDGE_BASE_API_URL not exist");
    // let source_name: String = std::env::var("TERMINUS_RECOMMEND_SOURCE_NAME").expect("TERMINUS_RECOMMEND_SOURCE_NAME not exist");
    let url: String = format!(
        "{}/{}/{}",
        knowledge_base_prefix, ENTRY_API_SUFFIX, entry_id
    );

    logdebug!("last url {}", url);

    let echo_json: serde_json::Value = reqwest::Client::new()
        .get(url)
        .send()
        .await
        .expect("send request fail")
        .json()
        .await
        .expect("parse result fail");

    // logdebug!("get entry by id  result {:#?}",echo_json);

    let current_code: i64 = echo_json
        .get("code")
        .expect("code not exists")
        .as_i64()
        .expect("code can not convert to i64");
    let message: Option<&serde_json::Value> = echo_json.get("message");
    let current_message_str: String;
    if let Some(current_message) = message {
        let current_convert_option = current_message.as_str();
        if let Some(temp_value) = current_convert_option {
            current_message_str = String::from(temp_value);
        } else {
            current_message_str = String::from("null");
        }
    } else {
        current_message_str = String::from("null");
    }

    logdebug!(
        "code {} current_message {}",
        current_code,
        current_message_str
    );
    let current_data: &serde_json::Value =
        echo_json.get("data").expect("response json have no data");
    let current_entry_option: Option<Entry> = convert_from_web_json_to_entry(current_data);

    return current_entry_option;
}

pub async fn get_impression_pagination(
    limit: i64,
    offset: i64,
    clicked: Option<bool>,
    impression_list: &mut Vec<Impression>,
    count: &mut i64,
    source_name: &String,
) {
    /*
    *Object {
     "code": Number(0),
     "message": Null,
     "data": Object {
         "count": Number(114),
         "offset": String("0"),
         "limit": String("1"),
         "items": Array [
             Object {
                 "batch_id": String("false"),
                 "position": String("false"),
                 "_id": String("659f9ccc584a4a82eaef3bb9"),
                 "entry_id": String("6555fc001d141d0bf0025595"),
                 "clicked": Bool(true),
                 "read_finish": Bool(true),
                 "stared": Bool(false),
                 "read_time": Number(75),
                 "algorithm_extra": Object {
                     "embedding": Array [
                         Number(-0.06527794897556305),
                         Number(0.03734969347715378),
    */
    let knowledge_base_prefix: String =
        std::env::var("KNOWLEDGE_BASE_API_URL").expect("KNOWLEDGE_BASE_API_URL not exist");
    // let source_name: String = std::env::var("TERMINUS_RECOMMEND_SOURCE_NAME").expect("TERMINUS_RECOMMEND_SOURCE_NAME not exist");
    let mut url: String = format!(
        "{}/{}?offset={}&limit={}&source={}",
        knowledge_base_prefix, IMPRESSION_API_SUFFIX, offset, limit, source_name
    );
    if let Some(current_clicked) = clicked {
        url = format!("{}&clicked={}", url, current_clicked)
    }
    logdebug!("last url {}", url);
    let echo_json: serde_json::Value = reqwest::Client::new()
        .get(url)
        .send()
        .await
        .expect("send request fail")
        .json()
        .await
        .expect("parse result fail");

    // logdebug!("get impression pagination result {:#?}",echo_json);
    let current_code: i64 = echo_json
        .get("code")
        .expect("code not exists")
        .as_i64()
        .expect("code can not convert to i64");
    let message: Option<&serde_json::Value> = echo_json.get("message");
    let current_message_str: String;
    if let Some(current_message) = message {
        let current_convert_option = current_message.as_str();
        if let Some(temp_value) = current_convert_option {
            current_message_str = String::from(temp_value);
        } else {
            current_message_str = String::from("null");
        }
    } else {
        current_message_str = String::from("null");
    }

    logdebug!(
        "code {} current_message {}",
        current_code,
        current_message_str
    );
    let current_data: &serde_json::Value =
        echo_json.get("data").expect("response json have no data");
    let current_count: i64 = current_data
        .get("count")
        .expect("response json have no count")
        .as_i64()
        .expect("convert count to i64 fail");
    *count = current_count;
    // count = &mut current_count;
    let items: &Vec<serde_json::Value> = current_data
        .get("items")
        .expect("get items fail")
        .as_array()
        .expect("get item array fail");
    logdebug!("item size {}", items.len());
    for current_item in items {
        let current_impression_option = convert_from_web_json_to_impression(current_item);
        if let Some(current_impression) = current_impression_option {
            impression_list.push(current_impression);
        }
    }
}

pub async fn get_user_embedding_str() -> (String, bool) {
    let knowledge_base_prefix: String =
        std::env::var("KNOWLEDGE_BASE_API_URL").expect("KNOWLEDGE_BASE_API_URL not exist");
    let source_name: String = std::env::var("TERMINUS_RECOMMEND_SOURCE_NAME")
        .expect("TERMINUS_RECOMMEND_SOURCE_NAME not exist");
    let url: String = format!(
        "{}/{}/{}/user_embedding",
        knowledge_base_prefix, CONFIG_API_SUFFIX, source_name
    );
    let echo_json: serde_json::Value = reqwest::Client::new()
        .get(url)
        .send()
        .await
        .expect("send request fail")
        .json()
        .await
        .expect("parse result fail");
    logdebug!("set user embedding result {:#?}", echo_json);
    let code = echo_json["code"].as_i64().expect("code not exist");
    if code != 0 {
        return (String::from(""), false);
    } else {
        let data = echo_json["data"].as_str().expect("get data fail");
        return (String::from(data), true);
    }
}
pub async fn set_user_embedding_str_through_knowledge(
    user_embedding_str: String,
    user_embedding_dimension: usize,
) -> bool {
    let knowledge_base_prefix: String =
        std::env::var("KNOWLEDGE_BASE_API_URL").expect("KNOWLEDGE_BASE_API_URL not exist");
    let source_name: String = std::env::var("TERMINUS_RECOMMEND_SOURCE_NAME")
        .expect("TERMINUS_RECOMMEND_SOURCE_NAME not exist");
    let url: String = format!(
        "{}/{}/{}/user_embedding",
        knowledge_base_prefix, CONFIG_API_SUFFIX, source_name
    );
    logdebug!("url {}", url);
    let parse_result =
        embedding_common::parse_user_embedding(&user_embedding_str, user_embedding_dimension);
    if parse_result.1 == false {
        logerror!("user embedding str is not valid, set in redis fail");
        return false;
    }

    logdebug!("user_embedding_str {}", user_embedding_str);
    let echo_json: serde_json::Value = reqwest::Client::new()
        .post(url)
        .json(&serde_json::json!({
            "value": user_embedding_str,
        }))
        .send()
        .await
        .expect("send request fail")
        .json()
        .await
        .expect("parse result fail");

    logdebug!("set user embedding result {:#?}", echo_json);

    return true;
}

#[cfg(test)]
mod knowledgebaseapitest {
    // Note this useful idiom: importing names from outer (for mod tests) scope.

    use super::*;
    use crate::common_test_operation;
    use crate::embedding_common;
    use crate::{bertv2, embedding_common::init_user_embedding};

    #[tokio::test]
    async fn test_set_user_embedding_str() {
        // common::init_logger();
        common_test_operation::init_env();
        let init_tensor = init_user_embedding(bertv2::BERTV2_EMBEDDING_DIMENSION);
        embedding_common::set_user_embedding_knowledgebase(&init_tensor).await;
        // set_user_embedding_str_through_knowledge(String::from("1.1;1.1;1.2;3"), 3).await;
        let result = get_user_embedding_str().await;
        println!("{:#?}", result);
    }

    #[tokio::test]
    async fn test_get_impression_pagination() {
        // cargo test knowledgebaseapitest::test_get_impression_pagination
        common_test_operation::init_env();
        // env::set_var("TERMINUS_RECOMMEND_SOURCE_NAME", "bert_v1");
        let mut impression_list: Vec<Impression> = Vec::new();
        let mut count: i64 = 0;
        get_impression_pagination(
            1,
            0,
            Some(true),
            &mut impression_list,
            &mut count,
            &String::from("bert_v2"),
        )
        .await;
        logdebug!(
            "impression_list size {} count {}",
            impression_list.len(),
            count
        );
    }

    #[tokio::test]
    async fn test_get_all_impression() {
        // cargo test knowledgebaseapitest::test_get_all_impression
        common_test_operation::init_env();
        let mut impression_list: Vec<Impression> = Vec::new();
        let mut count: i64 = 0;
        get_all_impression(
            Some(true),
            &mut impression_list,
            &mut count,
            &String::from("wise"),
        )
        .await;
        logdebug!(
            "impression_list size {} count {}",
            impression_list.len(),
            count
        );
    }

    #[tokio::test]
    async fn test_get_entry_by_id() {
        // cargo test knowledgebaseapitest::test_get_entry_by_id
        common_test_operation::init_env();
        let test_entry_id = "6555bef91d141d0bf00224ec";
        get_entry_by_id(test_entry_id).await;
    }
}
