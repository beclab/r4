#include "common_tool.h"
#include "knowledgebase_api.h"

#include <cpprest/json.h>

#include <optional>
#include <iostream>
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdlib>

#include "easylogging++.h"

#include <boost/date_time.hpp>
#include "http_single_client.h"

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

  std::vector<std::pair<std::string, float>> rankScoreMetadata(const std::unordered_map<std::string, ScoreWithMetadata> &algorithm_id_to_score_with_meta)
  {
    std::vector<std::pair<std::string, float>> result;
    for (const auto &pr : algorithm_id_to_score_with_meta)
    {
      result.push_back(std::make_pair(pr.first, pr.second.score));
    }
    std::sort(result.begin(), result.end(), [](const std::pair<std::string, float> &a, const std::pair<std::string, float> &b)
              {
                return a.second > b.second; // Compare the second element of the pair, in descending order
              });
    return result;
  }

  bool updateAlgorithmScoreAndMetadataWithScoreOrder(
      const std::unordered_map<std::string, ScoreWithMetadata> &algorithm_id_to_score_with_meta)
  {

    // LOG(DEBUG) << "algorithm url " <<
    // concat_prefix_and_suffix_get_url(algorithm_api_suffix) << std::endl;
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;

    web::json::value algorithm_list;

    std::vector<std::pair<std::string, float>> ordered_algorithmed_id = rankScoreMetadata(algorithm_id_to_score_with_meta);

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
    int sequence_index = 0;
    for (const auto &current_algorithm_id_score_pair : ordered_algorithmed_id)
    {
      std::pair<std::string, ScoreWithMetadata> current_item = *algorithm_id_to_score_with_meta.find(current_algorithm_id_score_pair.first);
      web::json::value current_algorithm;
      current_algorithm[ALGORITHM_MONGO_FIELD_ID] = web::json::value::string(current_item.first);
      current_algorithm[ALGORITHM_MONGO_FIELD_SCORE] = web::json::value::number(current_item.second.score);
      current_algorithm[ALGORITHM_MONGO_FIELD_RANKED] = web::json::value::boolean(true);
      int64_t current_time = current_item.second.score_rank_time;
      current_algorithm[ALGORITHM_MONGO_FIELD_SCORE_RANK_TIME] = web::json::value::number(current_time);
      current_algorithm[ALGORITHM_MONGO_FIELD_SCORE_RANK_METHOD] = web::json::value::string(current_item.second.score_rank_method);
      current_algorithm[ALGORITHM_MONGO_FIELD_SCORE_RANK_SEQUENCE] = web::json::value::number(sequence_index);
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
      sequence_index++;
    }

    if (index > 0)
    {
      send(algorithm_list);
    }

    return true;
  }

  bool updateAlgorithmScoreAndMetadata(
      const std::unordered_map<std::string, ScoreWithMetadata> &algorithm_id_to_score_with_meta)
  {
    // LOG(DEBUG) << "algorithm url " <<
    // concat_prefix_and_suffix_get_url(algorithm_api_suffix) << std::endl;
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;

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
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client *current_client = HttpClientSingleton::get_instance();
    http_client &client = *current_client;

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
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;

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
    // std::cout << "************************ " << current_item << std::endl;
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
      // return std::nullopt;
    }

    if (current_item.has_string_field(ENTRY_LANGUAGE))
    {
      temp_entry.language = current_item.at(ENTRY_LANGUAGE).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_LANGUAGE
                 << std::endl;
      // return std::nullopt;
    }

    if (current_item.has_string_field(ENTRY_URL))
    {
      temp_entry.url = current_item.at(ENTRY_URL).as_string();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_URL << std::endl;
      // return std::nullopt;
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
      // return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_READ_LATER))
    {
      temp_entry.readlater = current_item.at(ENTRY_READ_LATER).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_READ_LATER
                 << std::endl;
      // return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_CRAWLER))
    {
      temp_entry.crawler = current_item.at(ENTRY_CRAWLER).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_CRAWLER
                 << std::endl;
      // return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_STARRED))
    {
      temp_entry.starred = current_item.at(ENTRY_STARRED).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_STARRED
                 << std::endl;
      // return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_DISABLED))
    {
      temp_entry.disabled = current_item.at(ENTRY_DISABLED).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_DISABLED
                 << std::endl;
      // return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_SAVED))
    {
      temp_entry.saved = current_item.at(ENTRY_SAVED).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_SAVED << std::endl;
      // return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_UNREAD))
    {
      temp_entry.unread = current_item.at(ENTRY_UNREAD).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_UNREAD
                 << std::endl;
      // return std::nullopt;
    }

    if (current_item.has_boolean_field(ENTRY_EXTRACT))
    {
      temp_entry.extract = current_item.at(ENTRY_EXTRACT).as_bool();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_EXTRACT
                 << std::endl;
      // return std::nullopt;
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
      // return std::nullopt;
    }
    if (current_item.has_integer_field(ENTRY_LAST_OPENED))
    {
      temp_entry.last_opened = current_item.at(ENTRY_LAST_OPENED).as_integer();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_LAST_OPENED
                 << std::endl;
      // return std::nullopt;
    }

    if (current_item.has_integer_field(ENTRY_PUBLISHED_AT))
    {
      temp_entry.published_at = current_item.at(ENTRY_PUBLISHED_AT).as_integer();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_PUBLISHED_AT
                 << std::endl;
      // return std::nullopt;
    }
    if (current_item.has_integer_field(ENTRY_INTEGER_ID))
    {
      temp_entry.integer_id = current_item.at(ENTRY_INTEGER_ID).as_integer();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ENTRY_INTEGER_ID
                 << std::endl;
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

    if (current_item.has_integer_field(ALGORITHM_MONGO_FIELD_INTEGER_ID))
    {
      temp_algorithm.integer_id = current_item.at(ALGORITHM_MONGO_FIELD_INTEGER_ID).as_integer();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << ALGORITHM_MONGO_FIELD_INTEGER_ID << std::endl;
      temp_algorithm.integer_id = 0;
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
    if (current_item.has_integer_field(ALGORITHM_MONGO_FIELD_SCORE_RANK_SEQUENCE))
    {
      temp_algorithm.score_rank_sequence =
          current_item.at(ALGORITHM_MONGO_FIELD_SCORE_RANK_SEQUENCE).as_integer();
    }
    else
    {
      temp_algorithm.score_rank_sequence = 0;
      LOG(ERROR) << "current web json value have no "
                 << ALGORITHM_MONGO_FIELD_SCORE_RANK_SEQUENCE << std::endl;
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

    if (current_item.has_double_field(ALGORITHM_MONGO_FIELD_SCORE) || current_item.has_integer_field(ALGORITHM_MONGO_FIELD_SCORE) || current_item.has_number_field(ALGORITHM_MONGO_FIELD_SCORE))
    {
      temp_algorithm.score =
          current_item.at(ALGORITHM_MONGO_FIELD_SCORE).as_double();
    }
    else if (current_item.has_string_field(ALGORITHM_MONGO_FIELD_SCORE))
    {
      temp_algorithm.score = stringToDouble(current_item.at(ALGORITHM_MONGO_FIELD_SCORE).as_string());
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

    if (current_item
            .has_number_field(ALGORITHM_MONGO_FIELD_SCORE_RANK_TIME))
    {
      int current_score_rank_time = current_item.at(ALGORITHM_MONGO_FIELD_SCORE_RANK_TIME).as_integer();
      temp_algorithm.score_rank_time = std::make_optional(current_score_rank_time);
    }
    else
    {
      temp_algorithm.score_rank_time = std::nullopt;
      LOG(DEBUG) << "temp_algorithm " << temp_algorithm.id << " have no "
                 << ALGORITHM_MONGO_FIELD_SCORE_RANK_TIME << std::endl;
    }

    if (current_item.has_string_field(ALGORITHM_MONGO_FIELD_SCORE_RANK_METHOD))
    {
      std::string score_rank_method =
          current_item.at(ALGORITHM_MONGO_FIELD_SCORE_RANK_METHOD).as_string();
      temp_algorithm.score_rank_method = std::make_optional(score_rank_method);
    }
    else
    {
      temp_algorithm.score_rank_method = "";
      LOG(DEBUG) << "temp_algorithm " << temp_algorithm.id << " have no "
                 << ALGORITHM_MONGO_FIELD_SCORE_RANK_METHOD << std::endl;
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

    if (current_item.has_integer_field(IMPRESSION_INTEGER_ID))
    {
      temp_impression.integer_id = current_item.at(IMPRESSION_INTEGER_ID).as_integer();
    }
    else
    {
      LOG(ERROR) << "current web json value have no " << IMPRESSION_INTEGER_ID
                 << std::endl;
      temp_impression.integer_id = 0;
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
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
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

  std::optional<std::string> getKnowledgeCofnig(const string &source, const string &key)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
    std::string current_config_api_suffix =
        std::string(CONFIG_API_SUFFIX) + "/" + source + "/" + key;

    LOG(DEBUG) << "current_config_api_suffix " << current_config_api_suffix
               << std::endl;

    uri_builder builder(U(current_config_api_suffix));

    std::string value;
    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&value](pplx::task<web::json::value> previousTask)
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
            value = v.at("data").as_string();
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return std::make_optional(value);
  }

  int64_t getLastExtractorTime(const std::string &source)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
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

  vector<double> getRecallUserEmbedding(const std::string &source)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
    std::string current_config_api_suffix =
        std::string(CONFIG_API_SUFFIX) + "/" + source + "/" + REDIS_KEY_RECALL_USER_EMBEDDING;

    LOG(DEBUG) << "current_config_api_suffix " << current_config_api_suffix
               << std::endl;

    uri_builder builder(U(current_config_api_suffix));

    vector<double> long_term_user_embedding;
    client.request(methods::GET, builder.to_string())
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&long_term_user_embedding](pplx::task<web::json::value> previousTask)
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
            std::string embedding = v.at("data").as_string();
            int embedding_dimension = getEnvInt(TERMINUS_RECOMMEND_EMBEDDING_DIMENSION, 384);

            long_term_user_embedding = parse_embedding(embedding, embedding_dimension);
       
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return long_term_user_embedding;
  }

  int64_t getLastRankTime(const std::string &source)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
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

  std::vector<double> parse_embedding(const std::string &input, size_t embedding_dimension)
  {
    std::vector<double> embedding_data;
    std::stringstream ss(input);
    std::string token;
    std::vector<std::string> split_result;

    // Split the input string by semicolon
    while (std::getline(ss, token, ';'))
    {
      split_result.push_back(token);
    }

    // Ensure there is at least one token, and the last token is a timestamp
    if (split_result.size() <= 1)
    {
      return {}; // Not enough tokens (embedding + timestamp)
    }

    // The last element should be the timestamp, remove it
    std::string timestamp_str = split_result.back();
    split_result.pop_back();

    // Validate that the last element is a valid timestamp (optional check)
    try
    {
      std::stoll(timestamp_str); // Try to convert timestamp to long long
    }
    catch (const std::invalid_argument &)
    {
      return {}; // If the timestamp is not valid, return empty
    }

    // If the embedding dimension doesn't match, return empty
    if (split_result.size() != embedding_dimension)
    {
      return {};
    }

    // Try to parse the embedding values as double
    for (const auto &element : split_result)
    {
      try
      {
        double value = std::stod(element); // Convert string to double
        embedding_data.push_back(value);
      }
      catch (const std::invalid_argument &)
      {
        return {}; // If any element is not a valid number, return empty
      }
    }

    return embedding_data;
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
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
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
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
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
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
    std::string current_suffix =
        std::string(ENTRY_API_SUFFIX) + "?offset=" + std::to_string(offset) + "&limit=" + std::to_string(limit) + "&source=" + source + "&extract=true" + "&fields=id,file_type,language,url,title,readlater,crawler,starred,disabled,saved,unread,extract,created_at,last_opened,published_at,integer_id";
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
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
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

  std::optional<Algorithm> GetAlgorithmByIntegerId(int integer_id)
  {
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
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
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

  void getAlgorithmAccordingImpression(int limit, int offset, std::string source,
                                       int impression,
                                       std::vector<Algorithm> *algorithm_list,
                                       int *count)
  {

    algorithm_list->clear();
    http_client *current_client = HttpClientSingleton::get_instance();
    std::cout << "current_client " << current_client << std::endl;
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
    std::string current_algorithm_api_suffix =
        std::string(ALGORITHM_API_SUFFIX) + "?offset=" + std::to_string(offset) +
        "&limit=" + std::to_string(limit) + "&source=" + source +
        "&impression=" + std::to_string(impression);
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

  bool updateRecallUserEmbedding(std::string source,
                                 const std::vector<double> &long_term_user_embedding, long long current_time)
  {
    std::string embedding_str;
    for (const auto &element : long_term_user_embedding)
    {
      embedding_str += std::to_string(element) + ";";
    }

    // Append the current timestamp to the end of the string
    embedding_str += std::to_string(getTimeStampNow());

    web::json::value current_value;
    current_value["value"] = web::json::value::string(embedding_str);
    return updateKnowledgeConfig(source, REDIS_KEY_RECALL_USER_EMBEDDING, current_value);
  }

  bool updateLastExtractorTime(std::string source, int64_t last_extractor_time)
  {
    web::json::value current_value;
    current_value["value"] = web::json::value::number(last_extractor_time);
    return updateKnowledgeConfig(source, LAST_EXTRACTOR_TIME, current_value);
  }

  bool postRecommendTraceUserEmbedding(const RecommendTraceUserEmbedding &embedding)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
    std::optional<web::json::value> optional_value =
        convertFromRecommendTraceUserEmbeddingToWebJsonValue(embedding);
    if (optional_value == std::nullopt)
    {
      LOG(ERROR) << "convertFromRecommendTraceUserEmbeddingToWebJsonValue failed"
                 << std::endl;
      return false;
    }
    web::json::value value = optional_value.value();
    const std::string current_user_embedding_api_suffix =
        std::string(RECOMMEND_TRACE_USER_EMBEDDING_API_SUFFIX);
    LOG(DEBUG) << "current_userembedding_api_prefix " << current_user_embedding_api_suffix
               << std::endl;
    std::string source = embedding.source;
    client.request(methods::POST, U(current_user_embedding_api_suffix), value)
        .then([source](http_response response) -> pplx::task<string_t>
              {
        LOG(DEBUG) << "create user embedding status_code "
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
          LOG(ERROR) << "create user embedding " << e.what()
                     << std::endl;
        } })
        .wait();

    return true;
  }

  bool postRecommendTraceInfo(const RecommendTraceInfo &info)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
    std::optional<web::json::value> optional_value =
        convertFromRecommendTraceInfoToWebJsonValue(info);
    if (optional_value == std::nullopt)
    {
      LOG(ERROR) << "convertFromRecommendTraceUserEmbeddingToWebJsonValue failed"
                 << std::endl;
      return false;
    }
    web::json::value value = optional_value.value();
    const std::string current_trace_info_api_suffix =
        std::string(RECOMMEND_TRACE_INFO_API_SUFFIX);
    LOG(DEBUG) << "current_trace_info_api_suffix " << current_trace_info_api_suffix
               << std::endl;
    std::string source = info.source;
    client.request(methods::POST, U(current_trace_info_api_suffix), value)
        .then([source](http_response response) -> pplx::task<string_t>
              {
        LOG(DEBUG) << "create recommend trace info status_code "
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
          LOG(ERROR) << "create recommend trace info " << e.what()
                     << std::endl;
        } })
        .wait();

    return true;
  }

  bool updateKnowledgeConfig(const std::string &source, const std::string &key,
                             const web::json::value &value)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    http_client &client = *current_client;
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

  std::vector<double> init_user_embedding(size_t embedding_dimension)
  {
    // Create a random number generator and normal distribution object
    std::random_device rd;                    // Used to obtain a random seed
    std::mt19937 gen(rd());                   // Random number generator
    std::normal_distribution<> dis(0.0, 1.0); // Gaussian distribution with mean 0 and standard deviation 1

    std::vector<double> result(embedding_dimension); // Store the output n-dimensional vector

    // Generate n random numbers to fill the vector
    for (int i = 0; i < embedding_dimension; ++i)
    {
      result[i] = dis(gen); // Sample from the Gaussian distribution
    }

    return result;
  }

  std::optional<web::json::value> convertFromRecommendTraceUserEmbeddingToWebJsonValue(
      const RecommendTraceUserEmbedding &embedding)
  {
    web::json::value result;
    if (embedding.source.empty())
    {
      LOG(ERROR) << "source is empty" << std::endl;
      return std::nullopt;
    }
    result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_SOURCE_FIELD] = web::json::value::string(embedding.source);

    result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_USER_EMBEDDING_FIELD] = web::json::value::array();
    web::json::value json_array = web::json::value::array();
    int index = 0;
    for (const auto &val : embedding.user_embedding)
    {
      json_array[index++] = web::json::value::number(val);
    }
    result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_USER_EMBEDDING_FIELD] = json_array;

    if (embedding.impression_id_used_to_calculate_embedding.empty())
    {
      LOG(ERROR) << "impression_id_used_to_calculate_embedding is empty" << std::endl;
      return std::nullopt;
    }
    result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_IMPRESSION_ID_USED_TO_CALCULATE_EMBEDDING_FIELD] = web::json::value::string(embedding.impression_id_used_to_calculate_embedding);

    if (embedding.unique_id.empty())
    {
      LOG(ERROR) << "unique_id is empty" << std::endl;
      return std::nullopt;
    }
    result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_UNIQUE_ID_FIELD] = web::json::value::string(embedding.unique_id);

    result[RECOMMEND_TRACE_INFO_USER_EMBEDDING_CREATED_RANK_TIME_FIELD] = web::json::value::number(int(embedding.created_rank_time));
    return std::make_optional(result);
  }

  std::optional<RecommendTraceUserEmbedding> convertFromWebJsonValueToRecommendTraceUserEmbedding(
      const web::json::value &value)
  {
    RecommendTraceUserEmbedding embedding;
    if (value.has_field(RECOMMEND_TRACE_INFO_USER_EMBEDDING_SOURCE_FIELD))
    {
      embedding.source = value.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_SOURCE_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "source is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_USER_EMBEDDING_USER_EMBEDDING_FIELD))
    {
      web::json::array user_embedding_array = value.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_USER_EMBEDDING_FIELD).as_array();
      for (const auto &val : user_embedding_array)
      {
        embedding.user_embedding.push_back(val.as_double());
      }
    }
    else
    {
      LOG(ERROR) << "user_embedding is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_USER_EMBEDDING_IMPRESSION_ID_USED_TO_CALCULATE_EMBEDDING_FIELD))
    {
      embedding.impression_id_used_to_calculate_embedding = value.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_IMPRESSION_ID_USED_TO_CALCULATE_EMBEDDING_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "impression_id_used_to_calculate_embedding is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_USER_EMBEDDING_UNIQUE_ID_FIELD))
    {
      embedding.unique_id = value.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_UNIQUE_ID_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "unique_id is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_USER_EMBEDDING_CREATED_RANK_TIME_FIELD))
    {
      embedding.created_rank_time = value.at(RECOMMEND_TRACE_INFO_USER_EMBEDDING_CREATED_RANK_TIME_FIELD).as_integer();
    }
    else
    {
      LOG(ERROR) << "created_rank_time is empty" << std::endl;
      return std::nullopt;
    }
    return std::make_optional(embedding);
  }

  std::optional<RecommendTraceUserEmbedding> findRecommendTraceUserEmbeddingByUniqueId(const std::string &unique_id)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    http_client &client = *current_client;
    std::string current_suffix = std::string(RECOMMEND_TRACE_USER_EMBEDDING_API_SUFFIX) + "/findByUniqueId/" + unique_id;
    LOG(DEBUG) << "current_suffix " << current_suffix << std::endl;

    std::optional<RecommendTraceUserEmbedding> option_embedding = std::nullopt;
    client.request(methods::GET, U(current_suffix))
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&option_embedding](pplx::task<web::json::value> previousTask)
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
            option_embedding = convertFromWebJsonValueToRecommendTraceUserEmbedding(current_value);
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return option_embedding;
  }

  std::optional<web::json::value> convertFromRecommendTraceInfoToWebJsonValue(
      const RecommendTraceInfo &info)
  {
    web::json::value result;
    if (info.source.empty())
    {
      LOG(ERROR) << "source is empty" << std::endl;
      return std::nullopt;
    }
    result[RECOMMEND_TRACE_INFO_SOURCE_FIELD] = web::json::value::string(info.source);

    if (info.rank_time <= 0)
    {
      LOG(ERROR) << "rank_time is invalid" << std::endl;
      return std::nullopt;
    }
    result[RECOMMEND_TRACE_INFO_RANK_TIME_FIELD] = web::json::value::number(int(info.rank_time));

    if (info.score_enum.empty())
    {
      LOG(ERROR) << "score_enum is empty" << std::endl;
      return std::nullopt;
    }

    result[RECOMMEND_TRACE_INFO_SCORE_ENUM_FIELD] = web::json::value::string(info.score_enum);

    result[RECOMMEND_TRACE_INFO_PREVIOUS_RANK_TIME_FIELD] = web::json::value::number(int(info.previous_rank_time));

    result[RECOMMEND_TRACE_INFO_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD] = web::json::value::string(info.not_impressioned_algorithm_id);

    result[RECOMMEND_TRACE_INFO_ADDED_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD] = web::json::value::string(info.added_not_impressioned_algorithm_id);

    result[RECOMMEND_TRACE_INFO_IMPRESSIONED_CLICKED_ID_FIELD] = web::json::value::string(info.impressioned_clicked_id);

    result[RECOMMEND_TRACE_INFO_ADDED_IMPRESSION_CLICKED_ID_FIELD] = web::json::value::string(info.added_impressioned_clicked_id);

    /**
    if (info.long_term_user_embedding_id.empty())
    {
      LOG(ERROR) << "long_term_user_embedding_id is empty" << std::endl;
      return std::nullopt;
    }
    */
    result[RECOMMEND_TRACE_INFO_LONG_TERM_USER_EMBEDDING_ID_FIELD] = web::json::value::string(info.long_term_user_embedding_id);

    /**
    if (info.short_term_user_embedding_id.empty())
    {
      LOG(ERROR) << "short_term_user_embedding_id is empty" << std::endl;
      return std::nullopt;
    }*/
    result[RECOMMEND_TRACE_INFO_SHORT_TERM_USER_EMBEDDING_ID_FIELD] = web::json::value::string(info.short_term_user_embedding_id);

    /**
    if (info.recall_user_embedding_id.empty())
    {
      LOG(ERROR) << "recall_user_embedding_id is empty" << std::endl;
      return std::nullopt;
    }*/
    result[RECOMMEND_TRACE_INFO_RECALL_USER_EMBEDDING_ID_FIELD] = web::json::value::string(info.recall_user_embedding_id);

    web::json::value top_ranked_algorithm_id_array = web::json::value::array();
    int index = 0;
    for (const auto &val : info.top_ranked_algorithm_id)
    {
      top_ranked_algorithm_id_array[index++] = web::json::value::number(val);
    }
    result[RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_ID_FIELD] = top_ranked_algorithm_id_array;

    index = 0;
    web::json::value top_ranked_algorithm_score_array = web::json::value::array();
    for (const auto &val : info.top_ranked_algorithm_score)
    {
      top_ranked_algorithm_score_array[index++] = web::json::value::number(val);
    }
    result[RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_SCORE_FIELD] = top_ranked_algorithm_score_array;
    result[RECOMMEND_TRACE_INFO_RECOMMEND_PARAMETER_JSON_SERIALIZED_FIELD] = web::json::value::string(info.recommend_parameter_json_serialized);
    return result;

  } // namespace knowledgebase

  std::optional<RecommendTraceInfo> convertFromWebJsonValueToRecommendTraceInfo(
      const web::json::value &value)
  {
    RecommendTraceInfo info;
    if (value.has_field(RECOMMEND_TRACE_INFO_SOURCE_FIELD))
    {
      info.source = value.at(RECOMMEND_TRACE_INFO_SOURCE_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "source is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_RANK_TIME_FIELD))
    {
      info.rank_time = value.at(RECOMMEND_TRACE_INFO_RANK_TIME_FIELD).as_integer();
    }
    else
    {
      LOG(ERROR) << "rank_time is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_PREVIOUS_RANK_TIME_FIELD))
    {
      info.previous_rank_time = value.at(RECOMMEND_TRACE_INFO_PREVIOUS_RANK_TIME_FIELD).as_integer();
    }
    else
    {
      LOG(ERROR) << "previous_rank_time is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_SCORE_ENUM_FIELD))
    {
      info.score_enum = value.at(RECOMMEND_TRACE_INFO_SCORE_ENUM_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "score_enum is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD))
    {
      info.not_impressioned_algorithm_id = value.at(RECOMMEND_TRACE_INFO_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "not_impressioned_algorithm_id is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_ADDED_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD))
    {
      info.added_not_impressioned_algorithm_id = value.at(RECOMMEND_TRACE_INFO_ADDED_NOT_IMPRESSIONED_ALGORITHM_ID_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "added_not_impressioned_algorithm_id is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_IMPRESSIONED_CLICKED_ID_FIELD))
    {
      info.impressioned_clicked_id = value.at(RECOMMEND_TRACE_INFO_IMPRESSIONED_CLICKED_ID_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "impressioned_clicked_id is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_ADDED_IMPRESSION_CLICKED_ID_FIELD))
    {
      info.added_impressioned_clicked_id = value.at(RECOMMEND_TRACE_INFO_ADDED_IMPRESSION_CLICKED_ID_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "added_impressioned_clicked_id is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_LONG_TERM_USER_EMBEDDING_ID_FIELD))
    {
      info.long_term_user_embedding_id = value.at(RECOMMEND_TRACE_INFO_LONG_TERM_USER_EMBEDDING_ID_FIELD).as_string();
    }
    else
    {
      info.long_term_user_embedding_id = "";
      LOG(ERROR) << "long_term_user_embedding_id is empty" << std::endl;
      // return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_SHORT_TERM_USER_EMBEDDING_ID_FIELD))
    {
      info.short_term_user_embedding_id = value.at(RECOMMEND_TRACE_INFO_SHORT_TERM_USER_EMBEDDING_ID_FIELD).as_string();
    }
    else
    {
      info.short_term_user_embedding_id = "";
      LOG(ERROR) << "short_term_user_embedding_id is empty" << std::endl;
      // return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_RECALL_USER_EMBEDDING_ID_FIELD))
    {
      info.recall_user_embedding_id = value.at(RECOMMEND_TRACE_INFO_RECALL_USER_EMBEDDING_ID_FIELD).as_string();
    }
    else
    {
      info.recall_user_embedding_id = "";
      LOG(ERROR) << "recall_user_embedding_id is empty" << std::endl;
      // return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_ID_FIELD))
    {
      web::json::array top_ranked_algorithm_id_array = value.at(RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_ID_FIELD).as_array();
      for (const auto &val : top_ranked_algorithm_id_array)
      {
        info.top_ranked_algorithm_id.push_back(val.as_integer());
      }
    }
    else
    {
      LOG(ERROR) << "top_ranked_algorithm_id is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_SCORE_FIELD))
    {
      web::json::array top_ranked_algorithm_score_array = value.at(RECOMMEND_TRACE_INFO_TOP_RANKED_ALGORITHM_SCORE_FIELD).as_array();
      for (const auto &val : top_ranked_algorithm_score_array)
      {
        info.top_ranked_algorithm_score.push_back(val.as_double());
      }
    }
    else
    {
      LOG(ERROR) << "top_ranked_algorithm_score is empty" << std::endl;
      return std::nullopt;
    }

    if (value.has_field(RECOMMEND_TRACE_INFO_RECOMMEND_PARAMETER_JSON_SERIALIZED_FIELD))
    {
      info.recommend_parameter_json_serialized = value.at(RECOMMEND_TRACE_INFO_RECOMMEND_PARAMETER_JSON_SERIALIZED_FIELD).as_string();
    }
    else
    {
      LOG(ERROR) << "created_time is empty" << std::endl;
      return std::nullopt;
    }

    return std::make_optional(info);
  }

  std::optional<RecommendTraceInfo> findRecommendTraceInfoByRankTimeAndSource(const std::string &source, int64_t rank_time)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    http_client &client = *current_client;
    std::string current_suffix = std::string(RECOMMEND_TRACE_INFO_API_SUFFIX) + "/findBySourceAndRankTime?source=" + source + "&rank_time" + std::to_string(rank_time);
    LOG(DEBUG) << "current_suffix " << current_suffix << std::endl;

    std::optional<RecommendTraceInfo> option_info = std::nullopt;
    client.request(methods::GET, U(current_suffix))
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&option_info](pplx::task<web::json::value> previousTask)
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
            option_info = convertFromWebJsonValueToRecommendTraceInfo(current_value);
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();
    return option_info;
  }

  std::vector<int> findAllRecomendTraceInfoRankTimesBySource(const std::string &source)
  {
    http_client *current_client = HttpClientSingleton::get_instance();
    http_client &client = *current_client;
    std::string current_suffix = std::string(RECOMMEND_TRACE_INFO_API_SUFFIX) + "/ranktimes/" + source;
    LOG(DEBUG) << "current_suffix " << current_suffix << std::endl;

    std::vector<int> rank_times;
    client.request(methods::GET, U(current_suffix))
        .then([](http_response response) -> pplx::task<web::json::value>
              {
        if (response.status_code() == status_codes::OK) {
          return response.extract_json();
        }
        return pplx::task_from_result(web::json::value()); })
        .then([&rank_times](pplx::task<web::json::value> previousTask)
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
            web::json::array rank_times_array = v.at("data").as_array();
            for (const auto &val : rank_times_array)
            {
              rank_times.push_back(val.as_integer());
            }
          }
        } catch (http_exception const &e) {
          LOG(ERROR) << "Error exception " << e.what() << std::endl;
        } })
        .wait();

    std::sort(rank_times.begin(), rank_times.end(), std::greater<int>());
    return rank_times;
  }

  void init_global_recommend_param_short_term_user_embedding_impression_count()
  {
    std::optional<std::string> short_term_user_embedding_impression_count_value = getKnowledgeCofnig(std::string(TERMINUS_RECOMMEND_PARAMETER), std::string(TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION));
    if (short_term_user_embedding_impression_count_value != std::nullopt && short_term_user_embedding_impression_count_value.value().length() > 0)
    {
      std::string short_term_user_embedding_impression_count_str = short_term_user_embedding_impression_count_value.value();
      try
      {
        globalTerminusRecommendParams.short_term_user_embedding_impression_count = std::stoi(short_term_user_embedding_impression_count_str);
        LOG(INFO) << "short_term_user_embedding_impression_count is set in redis, use [" << globalTerminusRecommendParams.short_term_user_embedding_impression_count << "]" << std::endl;
      }
      catch (std::invalid_argument const &e)
      {
        LOG(ERROR) << "short_term_user_embedding_impression_count Bad input: std::invalid_argument thrown [" << short_term_user_embedding_impression_count_str << "]" << std::endl;
      }
      catch (std::out_of_range const &e)
      {
        LOG(ERROR) << "short_term_user_embedding_impression_count Integer overflow: std::out_of_range thrown [" << short_term_user_embedding_impression_count_str << "]" << std::endl;
      }
    }
    else
    {
      LOG(INFO) << "short_term_user_embedding_impression_count is not set in redis, use" << std::endl;
    }
  }

  void init_global_recommend_param_long_term_user_embedding_impression_count()
  {
    std::optional<std::string> long_term_user_embedding_impression_count_value = getKnowledgeCofnig(std::string(TERMINUS_RECOMMEND_PARAMETER), std::string(TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION));
    if (long_term_user_embedding_impression_count_value != std::nullopt && long_term_user_embedding_impression_count_value.value().length() > 0)
    {
      std::string long_term_user_embedding_impression_count_str = long_term_user_embedding_impression_count_value.value();
      try
      {
        globalTerminusRecommendParams.long_term_user_embedding_impression_count = std::stoi(long_term_user_embedding_impression_count_str);
        LOG(INFO) << "long_term_user_embedding_impression_count is set in redis, use [" << globalTerminusRecommendParams.long_term_user_embedding_impression_count << "]" << std::endl;
      }
      catch (std::invalid_argument const &e)
      {
        LOG(ERROR) << "long_term_user_embedding_impression_count Bad input: std::invalid_argument thrown [" << long_term_user_embedding_impression_count_str << "]" << std::endl;
      }
      catch (std::out_of_range const &e)
      {
        LOG(ERROR) << "long_term_user_embedding_impression_count Integer overflow: std::out_of_range thrown [" << long_term_user_embedding_impression_count_str << "]" << std::endl;
      }
    }
    else
    {
      LOG(INFO) << "long_term_user_embedding_impression_count is not set in redis, use" << std::endl;
    }
  }

  void init_global_recommend_param_long_term_user_embedding_weight_for_rankscore()
  {
    std::optional<std::string> long_term_user_embedding_weight_for_rankscore_value = getKnowledgeCofnig(std::string(TERMINUS_RECOMMEND_PARAMETER), std::string(TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE));
    if (long_term_user_embedding_weight_for_rankscore_value != std::nullopt && long_term_user_embedding_weight_for_rankscore_value.value().length() > 0)
    {
      std::string long_term_user_embedding_weight_for_rankscore_str = long_term_user_embedding_weight_for_rankscore_value.value();
      try
      {
        globalTerminusRecommendParams.long_term_user_embedding_weight_for_rankscore = std::stod(long_term_user_embedding_weight_for_rankscore_str);
        LOG(INFO) << "long_term_user_embedding_weight_for_rankscore is set in redis, use [" << globalTerminusRecommendParams.long_term_user_embedding_weight_for_rankscore << "]" << std::endl;
      }
      catch (std::invalid_argument const &e)
      {
        LOG(ERROR) << "long_term_user_embedding_weight_for_rankscore Bad input: std::invalid_argument thrown [" << long_term_user_embedding_weight_for_rankscore_str << "]" << std::endl;
      }
      catch (std::out_of_range const &e)
      {
        LOG(ERROR) << "long_term_user_embedding_weight_for_rankscore Integer overflow: std::out_of_range thrown [" << long_term_user_embedding_weight_for_rankscore_str << "]" << std::endl;
      }
    }
    else
    {
      LOG(INFO) << "long_term_user_embedding_weight_for_rankscore is not set in redis, use" << std::endl;
    }
  }

  void init_global_recommend_param_article_time_weight_for_rankscore()
  {
    std::optional<std::string> article_time_weight_for_rankscore_value = getKnowledgeCofnig(std::string(TERMINUS_RECOMMEND_PARAMETER), std::string(TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE));
    if (article_time_weight_for_rankscore_value != std::nullopt && article_time_weight_for_rankscore_value.value().length() > 0)
    {
      std::string article_time_weight_for_rankscore_str = article_time_weight_for_rankscore_value.value();
      try
      {
        globalTerminusRecommendParams.article_time_weight_for_rankscore = std::stod(article_time_weight_for_rankscore_str);
        LOG(INFO) << "article_time_weight_for_rankscore is set in redis, use [" << globalTerminusRecommendParams.article_time_weight_for_rankscore << "]" << std::endl;
      }
      catch (std::invalid_argument const &e)
      {
        LOG(ERROR) << "article_time_weight_for_rankscore Bad input: std::invalid_argument thrown [" << article_time_weight_for_rankscore_str << "]" << std::endl;
      }
      catch (std::out_of_range const &e)
      {
        LOG(ERROR) << "article_time_weight_for_rankscore Integer overflow: std::out_of_range thrown [" << article_time_weight_for_rankscore_str << "]" << std::endl;
      }
    }
    else
    {
      LOG(INFO) << "article_time_weight_for_rankscore is not set in redis, use" << std::endl;
    }
  }

  void init_global_recommend_param_cold_start_article_clicked_number_threshold()
  {
    std::optional<std::string> cold_start_article_clicked_number_threshold_value = getKnowledgeCofnig(std::string(TERMINUS_RECOMMEND_PARAMETER), std::string(TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD));
    if (cold_start_article_clicked_number_threshold_value != std::nullopt && cold_start_article_clicked_number_threshold_value.value().length() > 0)
    {
      std::string cold_start_article_clicked_number_threshold_str = cold_start_article_clicked_number_threshold_value.value();
      try
      {
        globalTerminusRecommendParams.cold_start_article_clicked_number_threshold = std::stoi(cold_start_article_clicked_number_threshold_str);
        LOG(INFO) << "cold_start_article_clicked_number_threshold is set in redis, use [" << globalTerminusRecommendParams.cold_start_article_clicked_number_threshold << "]" << std::endl;
      }
      catch (std::invalid_argument const &e)
      {
        LOG(ERROR) << "cold_start_article_clicked_number_threshold Bad input: std::invalid_argument thrown [" << cold_start_article_clicked_number_threshold_str << "]" << std::endl;
      }
      catch (std::out_of_range const &e)
      {
        LOG(ERROR) << "cold_start_article_clicked_number_threshold Integer overflow: std::out_of_range thrown [" << cold_start_article_clicked_number_threshold_str << "]" << std::endl;
      }
    }
    else
    {
      LOG(INFO) << "cold_start_article_clicked_number_threshold is not set in redis, use" << std::endl;
    }
  }

  void init_global_recommend_param_long_or_short_embedding_as_recall_embedding()
  {
    std::optional<std::string> long_or_short_embedding_as_recall_embedding_value = getKnowledgeCofnig(std::string(TERMINUS_RECOMMEND_PARAMETER), std::string(TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING));
    if (long_or_short_embedding_as_recall_embedding_value != std::nullopt && long_or_short_embedding_as_recall_embedding_value.value().length() > 0)
    {
      std::string long_or_short_embedding_as_recall_embedding_str = long_or_short_embedding_as_recall_embedding_value.value();
      try
      {
        globalTerminusRecommendParams.long_or_short_embedding_as_recall_embedding = long_or_short_embedding_as_recall_embedding_str;
        LOG(INFO) << "long_or_short_embedding_as_recall_embedding is set in redis, use [" << globalTerminusRecommendParams.long_or_short_embedding_as_recall_embedding << "]" << std::endl;
      }
      catch (std::invalid_argument const &e)
      {
        LOG(ERROR) << "long_or_short_embedding_as_recall_embedding Bad input: std::invalid_argument thrown [" << long_or_short_embedding_as_recall_embedding_str << "]" << std::endl;
      }
      catch (std::out_of_range const &e)
      {
        LOG(ERROR) << "long_or_short_embedding_as_recall_embedding Integer overflow: std::out_of_range thrown [" << long_or_short_embedding_as_recall_embedding_str << "]" << std::endl;
      }
    }
    else
    {
      LOG(INFO) << "long_or_short_embedding_as_recall_embedding is not set in redis, use" << std::endl;
    }
  }

  void init_global_recommend_param_trace_info_number()
  {

    std::optional<std::string> trace_info_number_value = getKnowledgeCofnig(std::string(TERMINUS_RECOMMEND_PARAMETER), std::string(TERMINUS_RECOMMEND_TRACE_INFO_NUMBER));
    if (trace_info_number_value != std::nullopt && trace_info_number_value.value().length() > 0)
    {
      std::string trace_info_number_str = trace_info_number_value.value();
      try
      {
        globalTerminusRecommendParams.trace_info_number_zip = std::stoi(trace_info_number_str);
        LOG(INFO) << "trace_info_number is set in redis, use [" << globalTerminusRecommendParams.trace_info_number_zip << "]" << std::endl;
      }
      catch (std::invalid_argument const &e)
      {
        LOG(ERROR) << "trace_info_number Bad input: std::invalid_argument thrown [" << trace_info_number_str << "]" << std::endl;
      }
      catch (std::out_of_range const &e)
      {
        LOG(ERROR) << "trace_info_number Integer overflow: std::out_of_range thrown [" << trace_info_number_str << "]" << std::endl;
      }
    }
    else
    {
      LOG(INFO) << "trace_info_number is not set in redis, use" << std::endl;
    }
  }
  void init_global_terminus_recommend_params()
  {
    init_global_recommend_param_short_term_user_embedding_impression_count();
    init_global_recommend_param_long_term_user_embedding_impression_count();
    init_global_recommend_param_long_term_user_embedding_weight_for_rankscore();
    init_global_recommend_param_article_time_weight_for_rankscore();
    init_global_recommend_param_cold_start_article_clicked_number_threshold();
    init_global_recommend_param_long_or_short_embedding_as_recall_embedding();
    init_global_recommend_param_trace_info_number();
  }

  web::json::value convertTerminusRecommendParamsToJsonValue(const TerminusRecommendParams &params)
  {
    // Create a JSON object
    web::json::value json_obj;

    // Add the members of the struct to the JSON object
    json_obj[TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION] = web::json::value::number(params.short_term_user_embedding_impression_count);
    json_obj[TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION] = web::json::value::number(params.long_term_user_embedding_impression_count);
    json_obj[TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE] = web::json::value::number(params.long_term_user_embedding_weight_for_rankscore);
    json_obj[TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE] = web::json::value::number(params.article_time_weight_for_rankscore);
    json_obj[TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD] = web::json::value::number(params.cold_start_article_clicked_number_threshold);
    json_obj[TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING] = web::json::value::string(params.long_or_short_embedding_as_recall_embedding);
    json_obj[TERMINUS_RECOMMEND_TRACE_INFO_NUMBER] = web::json::value::number(params.trace_info_number_zip);

    return json_obj;
  }

}
