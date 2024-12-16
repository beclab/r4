#include "knowledgebase_api.h"

#include <cpprest/json.h>

#include <optional>
#include <iostream>
#include <chrono>
#include <thread>

#include "easylogging++.h"

#include <boost/date_time.hpp>

using namespace web::json;

using std::string;
using std::unordered_map;
using std::vector;

namespace knowledgebase
{

  namespace
  {

    boost::posix_time::ptime parse_time(string &timestamp_str)
    {
      timestamp_str[10] = ' ';
      timestamp_str.pop_back(); // remove the last 'Z'

      try
      {
        return boost::posix_time::time_from_string(timestamp_str);
      }
      catch (boost::bad_lexical_cast &e)
      {
        LOG(ERROR) << "Failed to convert timestamp " << timestamp_str << ", Reason: " << e.what();
        return boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1));
      }
    }

  } // namespace
  EntryCache::EntryCache() : cache_miss(0), cache_hit(0)
  {
  }

  EntryCache &EntryCache::getInstance()
  {
    static EntryCache instance;
    return instance;
  }

  void EntryCache::init()
  {
    loadAllEntries();
  }

  std::optional<Entry> EntryCache::getEntryById(const std::string &id)
  {
    if (cache.find(id) != cache.end())
    {
      ++cache_hit;
      return std::make_optional(cache[id]);
    }
    auto result = GetEntryById(id);
    if (result != std::nullopt)
    {
      ++cache_hit;
      cache[id] = result.value();
      return result;
    }
    ++cache_miss;
    return std::nullopt;
  }

  void EntryCache::loadAllEntries()
  {
    cache = getEntries(FLAGS_recommend_source_name);
    long long min_last_opened = std::numeric_limits<long long>::max();
    long long max_last_opened = 0;
    for (const auto &entry : cache)
    {
      if (entry.second.last_opened != 0 && entry.second.last_opened < min_last_opened)
      {
        min_last_opened = entry.second.last_opened;
      }
      if (entry.second.last_opened > max_last_opened)
      {
        max_last_opened = entry.second.last_opened;
      }
    }
    this->min_last_opened = min_last_opened;
    this->max_last_opened = max_last_opened;
  }

  void EntryCache::dumpStatistics()
  {
    LOG(INFO) << "Cache hit: " << cache_hit << ", miss: " << cache_miss << endl;
  }

  bool updateAlgorithmScoreAndMetadata(
      const std::unordered_map<std::string, ScoreWithMetadata> &algorithm_id_to_score_with_meta)
  {
    // LOG(DEBUG) << "algorithm url " <<
    // concat_prefix_and_suffix_get_url(algorithm_api_suffix) << std::endl;
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));

    web::json::value algorithm_list;

    auto send = [&client](const web::json::value &payload)
    {
      bool success = false;
      while (!success)
      {
        client.request(methods::POST, U(ALGORITHM_API_SUFFIX), payload)
            .then([](http_response response) -> pplx::task<string_t>
                  {
            if (response.status_code() == status_codes::Created) {
              return response.extract_string();
            }
            return pplx::task_from_result(string_t()); })
            .then([&](pplx::task<string_t> previousTask)
                  {
            try {
              string_t const &result = previousTask.get();
              success = true;
              // LOG(DEBUG) << result << endl;
            } catch (http_exception const &e) {
              // printf("Error exception:%s\n", e.what());
              LOG(ERROR) << "updateAlgorithmScoreAndMetadata Error exception "
                         << e.what() << std::endl;
            } })
            .wait();
        if (!success)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
          LOG(INFO) << "updateAlgorithmScoreAndMetadata retrying..." << std::endl;
        }
        else
        {
          LOG(INFO) << "updateAlgorithmScoreAndMetadata succeed." << std::endl;
        }
      }
    };

    size_t index = 0, batch_limit = 100;
    for (const auto &current_item : algorithm_id_to_score_with_meta)
    {
      web::json::value current_algorithm;
      current_algorithm["id"] = web::json::value::string(current_item.first);
      current_algorithm["score"] = web::json::value::number(current_item.second.score);
      current_algorithm["ranked"] = web::json::value::boolean(true);
      auto &extra = current_algorithm["extra"] = web::json::value();
      // TODO(haochengwang): Keyword as a reason here
      if (current_item.second.rankExecuted)
      {
        extra["reason_type"] = web::json::value::string("ARTICLE");
        auto &reason_data = extra["reason_data"] = web::json::value();
        auto &articles = current_item.second.reason.articles;
        for (int i = 0; i < articles.size(); ++i)
        {
          reason_data[i]["id"] = web::json::value::string(articles[i].id);
          reason_data[i]["title"] = web::json::value::string(articles[i].title);
          reason_data[i]["url"] = web::json::value::string(articles[i].url);
        }
      }
      algorithm_list[index++] = current_algorithm;

      if (index >= batch_limit)
      {
        send(algorithm_list);
        index = 0;
        algorithm_list = web::json::value();
      }
    }

    if (index > 0)
    {
      send(algorithm_list);
    }

    return true;
  }

  bool updateAlgorithmScoreAndRanked(const std::string &entry_id,
                                     float rank_score, bool ranked)
  {
    // LOG(DEBUG) << "algorithm url " <<
    // concat_prefix_and_suffix_get_url(algorithm_api_suffix) << std::endl;
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));

    web::json::value current_algorithm;
    current_algorithm["id"] = web::json::value::string(entry_id);
    current_algorithm["score"] = web::json::value::number(rank_score);
    current_algorithm["ranked"] = web::json::value::boolean(ranked);

    web::json::value algorithm_list;
    algorithm_list[0] = current_algorithm;

    LOG(DEBUG) << algorithm_list.to_string() << std::endl;

    client.request(methods::POST, U(ALGORITHM_API_SUFFIX), algorithm_list)
        .then([entry_id](http_response response) -> pplx::task<string_t>
              {
        LOG(DEBUG) << "update entry entry " << entry_id << " "
                   << response.status_code() << std::endl;
        if (response.status_code() == status_codes::Created) {
          return response.extract_string();
        }
        return pplx::task_from_result(string_t()); })
        .then([](pplx::task<string_t> previousTask)
              {
        try {
          string_t const &result = previousTask.get();
          LOG(DEBUG) << result << endl;
        } catch (http_exception const &e) {
          // printf("Error exception:%s\n", e.what());
          LOG(ERROR) << "updateAlgorithmScoreAndRanked Error exception "
                     << e.what() << std::endl;
        } })
        .wait();
    return true;
  }

  bool rerank(const std::string &source)
  {
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));

    uri_builder builder(U("knowledge/algorithm/reRank/" + source));

    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        // if the status is OK extract the body of the response into a JSON
        // value works only when the content type is application\json
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }

        // return an empty JSON value
        return pplx::task_from_result(web::json::value()); })
        // continue when the JSON value is available
        .then([](pplx::task<web::json::value> previousTask)
              {
        // get the JSON value from the task and display content from it
        try {
          web::json::value const &v = previousTask.get();
          // print_results(v);
          LOG(DEBUG) << "rerank " << v << std::endl;
        } catch (http_exception const &e) {
          // printf("Error exception:%s\n", e.what());
          LOG(ERROR) << "rerank error exception " << e.what() << std::endl;
        } })
        .wait();
  }

  std::optional<Entry> convertFromWebJsonValueToEntry(
      web::json::value current_item)
  {
    std::cout << "************************ " << current_item.at(ENTRY_ID) << std::endl;
    Entry temp_entry;
    if (current_item.is_null())
    {
      return std::nullopt;
    }

    if (current_item.has_string_field(ENTRY_ID))
    {
      temp_entry.id = current_item.at(ENTRY_ID).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_ID << std::endl;
      return std::nullopt;
    }

    if (current_item.has_string_field(ENTRY_FILE_TYPE))
    {
      temp_entry.file_type = current_item.at(ENTRY_FILE_TYPE).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_FILE_TYPE
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_string_field(ENTRY_LANGUAGE))
    {
      temp_entry.language = current_item.at(ENTRY_LANGUAGE).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_LANGUAGE
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_string_field(ENTRY_URL))
    {
      temp_entry.url = current_item.at(ENTRY_URL).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_URL << std::endl;
      return std::nullopt;
    }

    /**
    if (current_item.has_string_field(ENTRY_PURE_CONTENT)) {
      temp_entry.pure_content = current_item.at(ENTRY_PURE_CONTENT).as_string();
    } else {
      LOG(ERROR) << "current web json value have no " << ENTRY_PURE_CONTENT
                 << std::endl;
      return std::nullopt;
    }
    */

    if (current_item.has_string_field(ENTRY_TITLE))
    {
      temp_entry.title = current_item.at(ENTRY_TITLE).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_TITLE << std::endl;
      return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_READ_LATER))
    {
      temp_entry.readlater = current_item.at(ENTRY_READ_LATER).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_READ_LATER
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_CRAWLER))
    {
      temp_entry.crawler = current_item.at(ENTRY_CRAWLER).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_CRAWLER
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_STARRED))
    {
      temp_entry.starred = current_item.at(ENTRY_STARRED).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_STARRED
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_DISABLED))
    {
      temp_entry.disabled = current_item.at(ENTRY_DISABLED).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_DISABLED
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_SAVED))
    {
      temp_entry.saved = current_item.at(ENTRY_SAVED).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_SAVED << std::endl;
      return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_UNREAD))
    {
      temp_entry.unread = current_item.at(ENTRY_UNREAD).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_UNREAD
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_EXTRACT))
    {
      temp_entry.extract = current_item.at(ENTRY_EXTRACT).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_EXTRACT
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_string_field(ENTRY_CREATED_AT))
    {
      auto timestamp_str = current_item.at(ENTRY_CREATED_AT).as_string();
      temp_entry.timestamp = parse_time(timestamp_str);
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_CREATED_AT
                 << std::endl;
      return std::nullopt;
    }
    if (current_item.has_integer_field(ENTRY_LAST_OPENED))
    {
      temp_entry.last_opened = current_item.at(ENTRY_LAST_OPENED).as_integer();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_LAST_OPENED
                 << std::endl;
      return std::nullopt;
    }

    return std::make_optional(temp_entry);
  }

  std::optional<Algorithm> convertFromWebJsonValueToAlgorithm(
      web::json::value current_item)
  {
    Algorithm temp_algorithm;
    if (current_item.is_null())
    {
      return std::nullopt;
    }

    if (current_item.has_string_field(ALGORITHM_MONGO_FIELD_ID))
    {
      temp_algorithm.id = current_item.at(ALGORITHM_MONGO_FIELD_ID).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ALGORITHM_MONGO_FIELD_ID
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_string_field(ALGORITHM_MONGO_FIELD_ENTRY))
    {
      temp_algorithm.entry =
          current_item.at(ALGORITHM_MONGO_FIELD_ENTRY).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no "
                 << ALGORITHM_MONGO_FIELD_ENTRY << std::endl;
      return std::nullopt;
    }

    if (current_item.has_object_field(ALGORITHM_MONGO_FIELD_EXTRA))
    {
      web::json::value current_algorithm_extra =
          current_item.at(ALGORITHM_MONGO_FIELD_EXTRA);
      if (current_algorithm_extra.has_array_field(
              ALGORITHM_MONGO_FIELD_EMBEDDING))
      {
        web::json::array embedding =
            current_algorithm_extra.at(ALGORITHM_MONGO_FIELD_EMBEDDING)
                .as_array();
        std::vector<double> current_embedding_vec;
        for (web::json::value current_value : embedding)
        {
          double current_double = current_value.as_double();
          current_embedding_vec.push_back(current_double);
        }
        temp_algorithm.embedding = std::make_optional(current_embedding_vec);
      }
      else
      {
        temp_algorithm.embedding = std::nullopt;
        LOG(DEBUG) << "temp algorithm " << temp_algorithm.id << " have no "
                   << ALGORITHM_MONGO_FIELD_EMBEDDING << std::endl;
      }

      if (current_algorithm_extra.has_double_field(
              ALGORITHM_MONGO_FIELD_PRERANK_SCORE) ||
          current_algorithm_extra.has_integer_field(ALGORITHM_MONGO_FIELD_PRERANK_SCORE))
      {
        temp_algorithm.prerank_score = std::make_optional(
            current_algorithm_extra.at(ALGORITHM_MONGO_FIELD_PRERANK_SCORE)
                .as_double());
      }
      else
      {
        temp_algorithm.prerank_score = std::nullopt;
        LOG(DEBUG) << "temp algorithm " << temp_algorithm.id << " have no "
                   << ALGORITHM_MONGO_FIELD_PRERANK_SCORE << std::endl;
      }
    }
    else
    {
      temp_algorithm.embedding = std::nullopt;
      LOG(DEBUG) << "temp_impression " << temp_algorithm.id << " have no "
                 << ALGORITHM_MONGO_FIELD_ENTRY << std::endl;
    }

    if (current_item.has_string_field(ALGORITHM_MONGO_FIELD_SOURCE))
    {
      temp_algorithm.source =
          current_item.at(ALGORITHM_MONGO_FIELD_SOURCE).as_string();
    }
    else
    {
      temp_algorithm.source = "";
      LOG(ERROR) << "current web json value have no "
                 << ALGORITHM_MONGO_FIELD_SOURCE << std::endl;
    }

    if (current_item.has_integer_field(ALGORITHM_MONGO_FIELD_IMPRESSION))
    {
      temp_algorithm.impression =
          current_item.at(ALGORITHM_MONGO_FIELD_IMPRESSION).as_integer();
    }
    else
    {
      temp_algorithm.impression = 0;
      LOG(ERROR) << "current web json value have no "
                 << ALGORITHM_MONGO_FIELD_IMPRESSION << std::endl;
    }

    if (current_item.has_double_field(ALGORITHM_MONGO_FIELD_SCORE) || current_item.has_integer_field(ALGORITHM_MONGO_FIELD_SCORE))
    {
      temp_algorithm.score =
          current_item.at(ALGORITHM_MONGO_FIELD_SCORE).as_double();
    }
    else
    {
      temp_algorithm.score = 0;
      LOG(ERROR) << "current web json value have no "
                 << ALGORITHM_MONGO_FIELD_SCORE << std::endl;
    }

    if (current_item.has_boolean_field(ALGORITHM_MONGO_FIELD_RANKED))
    {
      temp_algorithm.ranked =
          current_item.at(ALGORITHM_MONGO_FIELD_RANKED).as_bool();
    }
    else
    {
      temp_algorithm.ranked = false;
      LOG(ERROR) << "current web json value have no "
                 << ALGORITHM_MONGO_FIELD_RANKED << std::endl;
    }

    return std::make_optional(temp_algorithm);
  }
  std::optional<Impression> convertFromWebJsonValueToImpression(
      web::json::value current_item)
  {
    // std::cout << "********************" << current_item.serialize() << std::endl;
    Impression temp_impression;
    if (current_item.is_null())
    {
      return std::nullopt;
    }

    if (current_item.has_string_field(IMPRESSION_MONGO_FIELD_ID))
    {
      temp_impression.id = current_item.at(IMPRESSION_MONGO_FIELD_ID).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << IMPRESSION_MONGO_FIELD_ID
                 << std::endl;
      return std::nullopt;
    }

    if (current_item.has_string_field(IMPRESSION_MONGO_FIELD_ENTRY_ID))
    {
      temp_impression.entry_id =
          current_item.at(IMPRESSION_MONGO_FIELD_ENTRY_ID).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no "
                 << IMPRESSION_MONGO_FIELD_ENTRY_ID << std::endl;
      return std::nullopt;
    }

    if (current_item.has_string_field(IMPRESSION_MONGO_FIELD_BATCH_ID))
    {
      temp_impression.batch_id =
          current_item.at(IMPRESSION_MONGO_FIELD_BATCH_ID).as_string();
    }
    else
    {
      temp_impression.batch_id = "";
      LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
                 << IMPRESSION_MONGO_FIELD_BATCH_ID << std::endl;
    }

    if (current_item.has_object_field(IMPRESSION_MONGO_FIELD_ALGORITHM_EXTRA))
    {
      web::json::value current_algorithm_extra =
          current_item.at(IMPRESSION_MONGO_FIELD_ALGORITHM_EXTRA);
      if (current_algorithm_extra.has_array_field(
              IMPRESSION_MONGO_FIELD_EMBEDDING))
      {
        web::json::array embedding =
            current_algorithm_extra.at(IMPRESSION_MONGO_FIELD_EMBEDDING)
                .as_array();
        std::vector<double> current_embedding_vec;
        for (web::json::value current_value : embedding)
        {
          double current_double = current_value.as_double();
          current_embedding_vec.push_back(current_double);
        }
        temp_impression.embedding = std::make_optional(current_embedding_vec);
      }
      else
      {
        LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
                   << IMPRESSION_MONGO_FIELD_EMBEDDING << std::endl;
      }
    }
    else
    {
      temp_impression.embedding = std::nullopt;
      LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
                 << IMPRESSION_MONGO_FIELD_ALGORITHM_EXTRA << std::endl;
    }

    if (current_item.has_boolean_field(IMPRESSION_MONGO_FIELD_CLICKED))
    {
      temp_impression.clicked =
          current_item.at(IMPRESSION_MONGO_FIELD_CLICKED).as_bool();
    }
    else
    {
      temp_impression.clicked = false;
      LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
                 << IMPRESSION_MONGO_FIELD_CLICKED << std::endl;
    }

    if (current_item.has_string_field(IMPRESSION_MONGO_FIELD_POSITION))
    {
      temp_impression.position =
          current_item.at(IMPRESSION_MONGO_FIELD_POSITION).as_string();
    }
    else
    {
      temp_impression.position = "";
      LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
                 << IMPRESSION_MONGO_FIELD_POSITION << std::endl;
    }

    if (current_item.has_boolean_field(IMPRESSION_MONGO_FIELD_READ_FINISH))
    {
      temp_impression.read_finish =
          current_item.at(IMPRESSION_MONGO_FIELD_READ_FINISH).as_bool();
    }
    else
    {
      temp_impression.read_finish = false;
      LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
                 << IMPRESSION_MONGO_FIELD_READ_FINISH << std::endl;
    }

    if (current_item.has_double_field(IMPRESSION_MONGO_FIELD_READ_TIME))
    {
      temp_impression.read_time =
          current_item.at(IMPRESSION_MONGO_FIELD_READ_TIME).as_double();
    }
    else
    {
      temp_impression.read_time = 0;
      // LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
      //            << IMPRESSION_MONGO_FIELD_READ_TIME << std::endl;
    }

    if (current_item.has_string_field(IMPRESSION_MONGO_FIELD_SOURCE))
    {
      temp_impression.source =
          current_item.at(IMPRESSION_MONGO_FIELD_SOURCE).as_string();
    }
    else
    {
      temp_impression.source = "";
      LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
                 << IMPRESSION_MONGO_FIELD_SOURCE << std::endl;
    }

    if (current_item.has_boolean_field(IMPRESSION_MONGO_FIELD_STARED))
    {
      temp_impression.stared =
          current_item.at(IMPRESSION_MONGO_FIELD_STARED).as_bool();
    }
    else
    {
      temp_impression.stared = false;
      LOG(DEBUG) << "temp_impression " << temp_impression.id << " have no "
                 << IMPRESSION_MONGO_FIELD_STARED << std::endl;
    }
    return std::make_optional(temp_impression);
  }

  void getImpression(int limit, int offset, std::string source,
                     std::vector<Impression> *impression_list, int *count)
  {
    /**
    {
      "code": 0,
      "message": null,
      "data": {
        "count": 4200,
        "offset": "2000",
        "limit": "10",
        "items": [
          {
            "batch_id": "false",
            "position": "false",
            "stared": false,
            "read_time": 0,
            "read_finish": false,
            "_id": "656d09f3a2e46f241f9a95c1",
            "entry_id": "653ec37d989405229166012f",
            "clicked": false,
            "algorithm_extra": {
              "embedding": [
                -0.08655105531215668,
                0.2607240378856659,
              ]
            }
        ]
      }
      */
    impression_list->clear();
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    std::string current_impression_api_suffix =
        std::string(IMPRESSION_API_SUFFIX) + "?offset=" + std::to_string(offset) +
        "&limit=" + std::to_string(limit) + "&source=" + source;
    LOG(DEBUG) << "current_impression_api_suffix "
               << current_impression_api_suffix << std::endl;

    uri_builder builder(U(current_impression_api_suffix));

    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&impression_list,
               &count](pplx::task<web::json::value> previousTask)
              {
        try {
          web::json::value const &v = previousTask.get();
          int code = v.at("code").as_integer();
          *count = v.at("data").at("count").as_integer();
          std::string message = "null";
          if (v.has_string_field("message")) {
            message = v.at("message").as_string();
          }
          LOG(DEBUG) << "code " << code << " message " << message << " count "
                     << count << std::endl;
          if (code == 0) {
            web::json::array list_item = v.at("data").at("items").as_array();
            for (web::json::value current_item : list_item) {
              std::optional<Impression> current_impression_option =
                  convertFromWebJsonValueToImpression(current_item);
              if (current_impression_option != std::nullopt) {
                auto current_temp = current_impression_option.value();
                impression_list->push_back(current_temp);
              }
            }
          }

          // print_results(v);
          // LOG(DEBUG) << "**v**" << v.to_string() << std::endl;
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
  }

  void convertStringTimestampToInt64(std::string str_timestamp,
                                     int64_t *int64_timestamp)
  {
    try
    {
      *int64_timestamp = std::stoll(str_timestamp);
      LOG(DEBUG) << "The number is: " << *int64_timestamp << std::endl;
    }
    catch (const std::invalid_argument &e)
    {
      LOG(ERROR) << "Invalid argument: " << str_timestamp << std::endl;
    }
    catch (const std::out_of_range &e)
    {
      LOG(ERROR) << "Out of range: " << str_timestamp << std::endl;
    }
  }

  int64_t getLastExtractorTime(const std::string &source)
  {
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    std::string current_config_api_suffix =
        std::string(CONFIG_API_SUFFIX) + "/" + source + "/" + LAST_EXTRACTOR_TIME;

    LOG(DEBUG) << "current_config_api_suffix " << current_config_api_suffix
               << std::endl;

    uri_builder builder(U(current_config_api_suffix));

    // Impression current_impression;
    int64_t last_extractor_time = -1;
    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([source,
               &last_extractor_time](pplx::task<web::json::value> previousTask)
              {
        try {
          web::json::value const &v = previousTask.get();
          int code = v.at("code").as_integer();
          std::string message = "null";
          if (v.has_string_field("message")) {
            message = v.at("message").as_string();
          }
          LOG(DEBUG) << "code " << code << " message " << message << std::endl;
          if (v.has_string_field("data")) {
            std::string last_rankt_time_data = v.at("data").as_string();
            convertStringTimestampToInt64(last_rankt_time_data,
                                          &last_extractor_time);
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return last_extractor_time;
  }

  int64_t getLastRankTime(const std::string &source)
  {
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    std::string current_config_api_suffix =
        std::string(CONFIG_API_SUFFIX) + "/" + source + "/" + LAST_RANK_TIME;

    LOG(DEBUG) << "current_config_api_suffix " << current_config_api_suffix
               << std::endl;

    uri_builder builder(U(current_config_api_suffix));

    // Impression current_impression;
    int64_t last_rank_time = -1;
    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([source,
               &last_rank_time](pplx::task<web::json::value> previousTask)
              {
        try {
          web::json::value const &v = previousTask.get();
          int code = v.at("code").as_integer();
          std::string message = "null";
          if (v.has_string_field("message")) {
            message = v.at("message").as_string();
          }
          LOG(DEBUG) << "code " << code << " message " << message << std::endl;
          if (v.has_string_field("data")) {
            std::string last_rankt_time_data = v.at("data").as_string();
            convertStringTimestampToInt64(last_rankt_time_data,
                                          &last_rank_time);
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return last_rank_time;
  }

  std::optional<Impression> GetImpressionById(const std::string &id)
  {
    /**
     * @brief
     * {
        "code": 0,
        "message": null,
        "data": {
            "batch_id": "false",
            "position": "false",
            "stared": false,
            "read_time": 0,
            "read_finish": false,
            "_id": "656d09e5a2e46f241f9a79fa",
            "entry_id": "653c9c75989405229165b10d",
            "clicked": false,
            "algorithm_extra": {},
            "source": "wise"
        }
        }
     */
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    std::string current_impression_api_suffix =
        std::string(IMPRESSION_API_SUFFIX) + "/" + id;

    LOG(DEBUG) << "current_impression_api_suffix "
               << current_impression_api_suffix << std::endl;

    uri_builder builder(U(current_impression_api_suffix));

    // Impression current_impression;
    std::optional<Impression> option_impression = std::nullopt;

    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&option_impression](pplx::task<web::json::value> previousTask)
              {
        try {
          web::json::value const &v = previousTask.get();
          int code = v.at("code").as_integer();
          std::string message = "null";
          if (v.has_string_field("message")) {
            message = v.at("message").as_string();
          }
          LOG(DEBUG) << "code " << code << " message " << message << std::endl;
          if (code == 0) {
            web::json::value current_value = v.at("data");
            option_impression =
                convertFromWebJsonValueToImpression(current_value);
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return option_impression;
  }

  std::optional<Entry> GetEntryById(const std::string &id)
  {
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    std::string current_entry_api_suffix =
        std::string(ENTRY_API_SUFFIX) + "/" + id;

    LOG(DEBUG) << "current_entry_api_suffix " << current_entry_api_suffix
               << std::endl;

    uri_builder builder(U(current_entry_api_suffix));

    // Impression current_impression;
    std::optional<Entry> option_entry = std::nullopt;

    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&option_entry](pplx::task<web::json::value> previousTask)
              {
        try {
          web::json::value const &v = previousTask.get();
          int code = v.at("code").as_integer();
          std::string message = "null";
          if (v.has_string_field("message")) {
            message = v.at("message").as_string();
          }
          LOG(DEBUG) << "code " << code << " message " << message << std::endl;
          if (code == 0) {
            web::json::value current_value = v.at("data");
            option_entry = convertFromWebJsonValueToEntry(current_value);
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return option_entry;
  }

  unordered_map<string, Entry> getEntries(const string &source)
  {
    unordered_map<string, Entry> result;
    int offset = 0, count = 0, limit = 100;
    do
    {
      vector<Entry> entry_list;
      getEntries(limit, offset, source, &entry_list, &count);
      for (auto &entry : entry_list)
      {
        result.emplace(entry.id, std::move(entry));
      }
      offset += limit;
    } while (result.size() < count);
    return result;
  }

  void getEntries(int limit, int offset, const string &source,
                  std::vector<Entry> *entry_list, int *count)
  {
    entry_list->clear();
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    std::string current_suffix =
        std::string(ENTRY_API_SUFFIX) + "?offset=" + std::to_string(offset) + "&limit=" + std::to_string(limit) + "&source=" + source + "&extract=true" + "&fields=id,file_type,language,url,title,readlater,crawler,starred,disabled,saved,unread,extract,created_at,last_opened";
    LOG(DEBUG) << "current_suffix " << current_suffix
               << std::endl;

    bool success = false;
    while (!success)
    {
      client.request(methods::GET, U(current_suffix))
          .then([](http_response response) -> pplx::task<web::json::value>
                {
                if (response.status_code() == status_codes::OK) {
                    return response.extract_json();
                }
                return pplx::task_from_result(web::json::value()); })
          .then([&](pplx::task<web::json::value> previousTask)
                {
                try {
                    web::json::value const &v = previousTask.get();
                    int code = v.at("code").as_integer();
                    std::string message = "null";
                    if (v.has_string_field("message")) {
                        message = v.at("message").as_string();
                    }
                    LOG(DEBUG) << "code " << code << " message " << message << std::endl;
                    if (code == 0) {
                        web::json::value data_value = v.at("data");
                        web::json::value items = data_value.at("items");
                        *count = data_value.at("count").as_integer();
                        for (auto iter = items.as_array().cbegin(); iter != items.as_array().cend(); ++iter) {
                            auto item = convertFromWebJsonValueToEntry(*iter);
                            if (item == std::nullopt) {
                                continue;
                            }
                            entry_list->push_back(item.value());
                        }
                    }
                    success = true;
                } catch (http_exception const &e) {
                    LOG(ERROR) << "Error exception " << e.what() << std::endl;
                } })
          .wait();

      if (!success)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(INFO) << "getEntries retrying..." << std::endl;
      }
      else
      {
        LOG(INFO) << "getEntries succeed." << std::endl;
      }
    }
  }

  std::optional<Algorithm> GetAlgorithmById(const std::string &id)
  {
    /**
     * @brief
     * {
  "code": 0,
  "message": null,
  "data": {
    "_id": "659f62bcd0158a3187db94bf",
    "entry": "6555bef91d141d0bf00224f9",
    "source": "bert_v2",
    "ranked": false,
    "score": 0,
    "extra": {
      "embedding": [
        0.1035594791173935,
        0.07692128419876099,
     */
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    std::string current_algorithm_api_suffix =
        std::string(ALGORITHM_API_SUFFIX) + "/" + id;

    LOG(DEBUG) << "current_impression_api_suffix " << current_algorithm_api_suffix
               << std::endl;

    uri_builder builder(U(current_algorithm_api_suffix));

    // Impression current_impression;
    std::optional<Algorithm> option_algorithm = std::nullopt;

    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&option_algorithm](pplx::task<web::json::value> previousTask)
              {
        try {
          web::json::value const &v = previousTask.get();
          int code = v.at("code").as_integer();
          std::string message = "null";
          if (v.has_string_field("message")) {
            message = v.at("message").as_string();
          }
          LOG(DEBUG) << "code " << code << " message " << message << std::endl;
          if (code == 0) {
            web::json::value current_value = v.at("data");
            option_algorithm =
                convertFromWebJsonValueToAlgorithm(current_value);
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return option_algorithm;
  }

  void getAllImpression(std::string source,
                        std::vector<Impression> *impression_list, int *count)
  {
    const int batch_size = 100;
    int offset = 0;
    while (true)
    {
      LOG(DEBUG) << "offset [" << offset << "] limit [" << batch_size << "] count ["
                 << *count << "]" << std::endl;
      std::vector<Impression> temp_impression;
      getImpression(batch_size, offset, source, &temp_impression, count);
      impression_list->insert(impression_list->end(), temp_impression.begin(),
                              temp_impression.end());
      offset = offset + batch_size;
      if (offset >= *count)
      {
        break;
      }
    }
  }

  void getAllAlgorithmAccordingRanked(std::string source,
                                      std::vector<Algorithm> *algorithm_list,
                                      bool ranked, int *count)
  {
    const int batch_size = 100;
    int offset = 0;
    while (true)
    {
      LOG(DEBUG) << "offset " << offset << " limit " << batch_size << " count "
                 << count;
      std::vector<Algorithm> temp_algorithm;
      getAlgorithmAccordingRanked(batch_size, offset, source, ranked,
                                  &temp_algorithm, count);
      algorithm_list->insert(algorithm_list->end(), temp_algorithm.begin(),
                             temp_algorithm.end());
      offset = offset + batch_size;
      if (offset >= *count)
      {
        break;
      }
    }
  }

  void getAlgorithmAccordingRanked(int limit, int offset, std::string source,
                                   bool ranked,
                                   std::vector<Algorithm> *algorithm_list,
                                   int *count)
  {
    /**
    {
      "code": 0,
      "message": null,
      "data": {
        "code": 0,
        "message": null,
        "data": {
          "count": 4960,
          "offset": "0",
          "limit": "100",
          "items": [
            {
              "_id": "656d09e5a2e46f241f9a7a5d",
              "entry": "653c9c75989405229165b10d",
              "source": "bert_v2",
              "ranked": true,
              "score": 0.1713230460882187,
              "extra": {
                "embedding": [
                  -0.018457941710948944,
                  0.02881704270839691,
                  -0.027788635343313217,
      */
    algorithm_list->clear();
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    std::string temp_ranked = "false";
    if (ranked)
    {
      temp_ranked = "true";
    }
    std::string current_algorithm_api_suffix =
        std::string(ALGORITHM_API_SUFFIX) + "?offset=" + std::to_string(offset) +
        "&limit=" + std::to_string(limit) + "&source=" + source +
        "&ranked=" + temp_ranked;
    LOG(DEBUG) << "current_algorithm_api_suffix " << current_algorithm_api_suffix
               << std::endl;

    uri_builder builder(U(current_algorithm_api_suffix));

    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&algorithm_list,
               &count](pplx::task<web::json::value> previousTask)
              {
        try {
          web::json::value const &temp = previousTask.get();
          web::json::value v = temp.at("data");
          int code = v.at("code").as_integer();
          // std::cout << v.to_string() << std::endl;
          *count = v.at("data").at("count").as_integer();
          std::string message = "null";
          if (v.has_string_field("message")) {
            message = v.at("message").as_string();
          }
          LOG(DEBUG) << "code " << code << " message " << message << " count "
                     << *count << std::endl;
          if (code == 0) {
            web::json::array list_item = v.at("data").at("items").as_array();
            for (web::json::value current_item : list_item) {
              std::optional<Algorithm> current_impression_option =
                  convertFromWebJsonValueToAlgorithm(current_item);
              if (current_impression_option != std::nullopt) {
                auto current_temp = current_impression_option.value();
                algorithm_list->push_back(current_temp);
              }
            }
          }

          // print_results(v);
          // LOG(DEBUG) << "**v**" << v.to_string() << std::endl;
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
  }

  bool updateLastRankTime(std::string source, int64_t last_rank_time)
  {
    web::json::value current_value;
    current_value["value"] = web::json::value::number(last_rank_time);
    return updateKnowledgeConfig(source, LAST_RANK_TIME, current_value);
  }

  bool updateLastExtractorTime(std::string source, int64_t last_extractor_time)
  {
    web::json::value current_value;
    current_value["value"] = web::json::value::number(last_extractor_time);
    return updateKnowledgeConfig(source, LAST_EXTRACTOR_TIME, current_value);
  }

  bool updateKnowledgeConfig(const std::string &source, const std::string &key,
                             const web::json::value &value)
  {
    http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));

    LOG(DEBUG) << "update source [" << source << "] key [" << key << "] "
               << value.to_string() << std::endl;

    const std::string current_config_api_suffix =
        std::string(CONFIG_API_SUFFIX) + "/" + source + "/" + key;
    LOG(DEBUG) << "current_config_api_suffix " << current_config_api_suffix
               << std::endl;

    client.request(methods::POST, U(current_config_api_suffix), value)
        .then([source, key](http_response response) -> pplx::task<string_t>
              {
        LOG(DEBUG) << "update source [" << source << "]   key [" << key << "] "
                   << response.status_code() << std::endl;
        if (response.status_code() == status_codes::Created) {
          return response.extract_string();
        }
        return pplx::task_from_result(string_t()); })
        .then([](pplx::task<string_t> previousTask)
              {
        try {
          string_t const &result = previousTask.get();
          LOG(DEBUG) << result << endl;
        } catch (http_exception const &e) {
          // printf("Error exception:%s\n", e.what());
          LOG(ERROR) << "updateKnowledgeConfig Error exception " << e.what()
                     << std::endl;
        } })
        .wait();

    return true;
  }
} // namespace knowledgebase
