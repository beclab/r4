#include "archive_file.h"
#include "knowledgebase_api.h"
#include "common_tool.h"
#include "dump_traceinfo.h"
#include "json.hpp"
#include <filesystem>
#include <xlnt/xlnt.hpp> // 引入 xlnt 库
#include <cmath>

using nlohmann::json;

/**
 * @brief
 * /opt/rank_model
 *       trace_log
 *           r4world
 *             1711080226
 *                 not_impressioned.xlsx
 *                 added_not_impressioned.xlsx
 *                 impressioned_clicked.xlsx
 *                 added_impressioned_clicked.xlsx
 *                 top_ranked_entry.xlsx
 *                 parameter.json
 *
 *             1711080228
 *
 *
 *           r4tech
 *
 */

std::optional<Entry> getEntryByImpressionIntegerId(int current_impression_integer_id)
{
    std::optional<Impression> current_impression = knowledgebase::GetImpressionByIntegerId(current_impression_integer_id);
    if (current_impression != std::nullopt)
    {
        Impression current_impression_value = current_impression.value();
        std::cout << current_impression_value.id << " " << current_impression_value.entry_id << std::endl;
        std::optional<Entry> current_entry = knowledgebase::EntryCache::getInstance().getEntryById(current_impression_value.entry_id);
        return current_entry;
    }
    return std::nullopt;
}

void writeWebJsonValueToFile(const web::json::value &web_json, const std::string &file_path)
{
    // Step 1: Serialize web::json::value to JSON string
    std::string web_json_string = utility::conversions::to_utf8string(web_json.serialize());

    // Step 2: Parse JSON string to nlohmann::json object
    nlohmann::json nlohmannJson = nlohmann::json::parse(web_json_string);

    // Step 3: Open a file output stream
    std::ofstream outFile(file_path);

    // Step 4: Format and output JSON data to file with 4-space indentation
    if (outFile.is_open())
    {
        outFile << nlohmannJson.dump(4); // Format output with 4-space indentation
        outFile.close();
        LOG(INFO) << "Successfully wrote info to: " << file_path << std::endl;
    }
    else
    {
        LOG(ERROR) << "Failed to write info to: " << file_path << std::endl;
    }
}
void writeEntryToExcel(const std::vector<Entry> &entries, const std::string &xlsx_path)
{
    // Create a workbook object
    xlnt::workbook wb;
    xlnt::worksheet ws = wb.active_sheet(); // Get the active worksheet

    // Write the header
    ws.cell("A1").value("IntegerId");
    ws.cell("B1").value("Title");
    ws.cell("C1").value("PublishedAt");
    ws.cell("D1").value("Url");
    ws.cell("E1").value("LastOpened");

    // Write the struct data row by row into Excel
    int row = 2; // Start writing from the second row
    for (const auto &current_entry : entries)
    {
        ws.cell("A" + std::to_string(row)).value(std::to_string(current_entry.integer_id));
        ws.cell("B" + std::to_string(row)).value(current_entry.title);
        ws.cell("C" + std::to_string(row)).value(std::to_string(current_entry.published_at));
        ws.cell("D" + std::to_string(row)).value(current_entry.url);
        ws.cell("E" + std::to_string(row)).value(std::to_string(current_entry.last_opened));
        row++; // Next row
    }
    wb.save(xlsx_path); // Save the workbook
}

void writeEntryToExcelWithScore(const std::vector<Entry> &entries, const std::string &xlsx_path, const vector<float> &score)
{
    if (entries.size() != score.size())
    {
        LOG(ERROR) << "for " << xlsx_path << " entries size " << entries.size() << " not equal to score size " << score.size() << std::endl;
        return;
    }
    // Create a workbook object
    xlnt::workbook wb;
    xlnt::worksheet ws = wb.active_sheet(); // Get the active worksheet

    // Write the header
    ws.cell("A1").value("IntegerId");
    ws.cell("B1").value("Title");
    ws.cell("C1").value("Score");
    ws.cell("D1").value("PublishedAt");
    ws.cell("E1").value("Url");
    ws.cell("F1").value("LastOpened");

    // Write the struct data row by row into Excel
    int row = 2; // Start writing from the second row
    for (int index = 0; index < entries.size(); index++)
    {
        const Entry &current_entry = entries[index];
        ws.cell("A" + std::to_string(row)).value(std::to_string(current_entry.integer_id));
        ws.cell("B" + std::to_string(row)).value(current_entry.title);
        ws.cell("C" + std::to_string(row)).value(std::to_string(score[index]));
        ws.cell("D" + std::to_string(row)).value(std::to_string(current_entry.published_at));
        ws.cell("E" + std::to_string(row)).value(current_entry.url);
        ws.cell("F" + std::to_string(row)).value(std::to_string(current_entry.last_opened));
        row++; // Next row
    }
    wb.save(xlsx_path); // Save the workbook
}

void write_not_impressioned_entry(const std::string &current_rank_time_path, const RecommendTraceInfo &current_trace_info_value)
{
    std::string not_impressioned_file_path = current_rank_time_path + "/" + NOT_IMRESSIONED_FILE_NAME;
    if (!std::filesystem::exists(not_impressioned_file_path))
    {
        std::vector<int> algorithm_integer_id = stringToArray(current_trace_info_value.not_impressioned_algorithm_id);
        std::sort(algorithm_integer_id.begin(), algorithm_integer_id.end(), std::greater<int>());
        std::vector<Entry> not_impressioned_entry_list;
        for (auto current_algorithm_integer_id : algorithm_integer_id)
        {
            std::optional<Algorithm> current_algorithm = knowledgebase::GetAlgorithmByIntegerId(current_algorithm_integer_id);
            if (current_algorithm != std::nullopt)
            {
                Algorithm current_algorithm_value = current_algorithm.value();
                std::cout << current_algorithm_value.id << " " << current_algorithm_value.entry << std::endl;
                std::optional<Entry> current_entry = knowledgebase::EntryCache::getInstance().getEntryById(current_algorithm_value.entry);

                if (current_entry != std::nullopt)
                {
                    not_impressioned_entry_list.push_back(current_entry.value());
                }
            }
        }
        writeEntryToExcel(not_impressioned_entry_list, not_impressioned_file_path);
    }
    else
    {
        LOG(DEBUG) << "not_impressioned_file_path " << not_impressioned_file_path << " already exist" << std::endl;
    }
}

void write_added_not_impressioned_entry(const std::string &current_rank_time_path, const RecommendTraceInfo &current_trace_info_value)
{
    std::string added_not_impressioned_file_path = current_rank_time_path + "/" + ADDED_NOT_IMRESSIONED_FILE_NAME;
    if (!std::filesystem::exists(added_not_impressioned_file_path))
    {
        std::vector<int> algorithm_integer_id = stringToArray(current_trace_info_value.added_not_impressioned_algorithm_id);
        std::sort(algorithm_integer_id.begin(), algorithm_integer_id.end(), std::greater<int>());
        std::vector<Entry> added_not_impressioned_entry_list;
        for (auto current_algorithm_integer_id : algorithm_integer_id)
        {
            std::optional<Algorithm> current_algorithm = knowledgebase::GetAlgorithmByIntegerId(current_algorithm_integer_id);
            if (current_algorithm != std::nullopt)
            {
                Algorithm current_algorithm_value = current_algorithm.value();
                std::cout << current_algorithm_value.id << " " << current_algorithm_value.entry << std::endl;
                std::optional<Entry> current_entry = knowledgebase::EntryCache::getInstance().getEntryById(current_algorithm_value.entry);

                if (current_entry != std::nullopt)
                {
                    added_not_impressioned_entry_list.push_back(current_entry.value());
                }
            }
        }
        writeEntryToExcel(added_not_impressioned_entry_list, added_not_impressioned_file_path);
    }
    else
    {
        LOG(DEBUG) << "added_not_impressioned_file_path " << added_not_impressioned_file_path << " already exist" << std::endl;
    }
}

void write_impressioned_clicked_entry(const std::string &current_rank_time_path, const RecommendTraceInfo &current_trace_info_value)
{
    std::string impressioned_clicked_file_path = current_rank_time_path + "/" + IMPRESSIONED_CLICKED_FILE_NAME;
    if (!std::filesystem::exists(impressioned_clicked_file_path))
    {
        std::vector<int> impression_integer_id = stringToArray(current_trace_info_value.impressioned_clicked_id);
        std::sort(impression_integer_id.begin(), impression_integer_id.end(), std::greater<int>());
        std::vector<Entry> impressioned_clicked_entry_list;
        for (auto current_impression_integer_id : impression_integer_id)
        {
            std::optional<Impression> current_impression = knowledgebase::GetImpressionByIntegerId(current_impression_integer_id);
            if (current_impression != std::nullopt)
            {
                Impression current_impression_value = current_impression.value();
                std::cout << current_impression_value.id << " " << current_impression_value.entry_id << std::endl;
                std::optional<Entry> current_entry = knowledgebase::EntryCache::getInstance().getEntryById(current_impression_value.entry_id);
                if (current_entry != std::nullopt)
                {
                    impressioned_clicked_entry_list.push_back(current_entry.value());
                }
            }
        }
        writeEntryToExcel(impressioned_clicked_entry_list, impressioned_clicked_file_path);
    }
    else
    {
        LOG(DEBUG) << "impressioned_clicked_file_path " << impressioned_clicked_file_path << " already exist" << std::endl;
    }
}

void write_added_impressioned_clicked_entry(const std::string &current_rank_time_path, const RecommendTraceInfo &current_trace_info_value)
{
    std::string added_impressioned_clicked_file_path = current_rank_time_path + "/" + ADDED_IMPRESSIONED_CLICKED_FILE_NAME;
    if (!std::filesystem::exists(added_impressioned_clicked_file_path))
    {
        std::vector<int> impression_integer_id = stringToArray(current_trace_info_value.added_impressioned_clicked_id);
        std::sort(impression_integer_id.begin(), impression_integer_id.end(), std::greater<int>());
        std::vector<Entry> added_impressioned_clicked_entry_list;
        for (auto current_impression_integer_id : impression_integer_id)
        {
            std::optional<Impression> current_impression = knowledgebase::GetImpressionByIntegerId(current_impression_integer_id);
            if (current_impression != std::nullopt)
            {
                Impression current_impression_value = current_impression.value();
                std::cout << current_impression_value.id << " " << current_impression_value.entry_id << std::endl;
                std::optional<Entry> current_entry = knowledgebase::EntryCache::getInstance().getEntryById(current_impression_value.entry_id);
                if (current_entry != std::nullopt)
                {
                    added_impressioned_clicked_entry_list.push_back(current_entry.value());
                }
            }
        }
        writeEntryToExcel(added_impressioned_clicked_entry_list, added_impressioned_clicked_file_path);
    }
    else
    {
        LOG(DEBUG) << "added_impressioned_clicked_file_path " << added_impressioned_clicked_file_path << " already exist" << std::endl;
    }
}

void write_top_ranked_entry(const std::string &current_rank_time_path, const RecommendTraceInfo &current_trace_info_value)
{
    std::string top_ranked_entry_file_path = current_rank_time_path + "/" + TOP_RANKED_ENTRY_FILE_NAME;
    if (!std::filesystem::exists(top_ranked_entry_file_path))
    {
        std::vector<int> algorithm_integer_id = current_trace_info_value.top_ranked_algorithm_id;
        // std::sort(algorithm_integer_id.begin(), algorithm_integer_id.end(), std::greater<int>());
        std::vector<float> algorithm_score = current_trace_info_value.top_ranked_algorithm_score;
        std::vector<Entry> top_ranked_entry_list;
        for (int index = 0; index < algorithm_integer_id.size(); index++)
        {
            int current_algorithm_integer_id = algorithm_integer_id[index];
            std::optional<Algorithm> current_algorithm = knowledgebase::GetAlgorithmByIntegerId(current_algorithm_integer_id);
            if (current_algorithm != std::nullopt)
            {
                Algorithm current_algorithm_value = current_algorithm.value();
                // std::cout << current_algorithm_value.id << " " << current_algorithm_value.entry << std::endl;
                std::optional<Entry> current_entry = knowledgebase::EntryCache::getInstance().getEntryById(current_algorithm_value.entry);
                if (current_entry != std::nullopt)
                {
                    top_ranked_entry_list.push_back(current_entry.value());
                }
            }
        }
        writeEntryToExcelWithScore(top_ranked_entry_list, top_ranked_entry_file_path, algorithm_score);
    }
    else
    {
        LOG(DEBUG) << "top_ranked_entry_file_path [" << top_ranked_entry_file_path << "] already exist" << std::endl;
    }
}

void write_recommend_parameter(const std::string &current_rank_time_path, const RecommendTraceInfo &current_trace_info_value)
{
    std::string parameter_file_path = current_rank_time_path + "/" + PARAMETER_FILE_NAME;
    LOG(DEBUG) << "parameter_file_path " << parameter_file_path << std::endl;
    if (!std::filesystem::exists(parameter_file_path))
    {
        std::string recommend_parameter_json_serialized = current_trace_info_value.recommend_parameter_json_serialized;
        web::json::value current_value = web::json::value::parse(recommend_parameter_json_serialized);
        // std::ofstream parameter_file(parameter_file_path);
        // current_value.serialize(parameter_file);
        // parameter_file.close();
        writeWebJsonValueToFile(current_value, parameter_file_path);
    }
    else
    {
        LOG(DEBUG) << "parameter_file_path [" << parameter_file_path << "] already exist" << std::endl;
    }
}

void write_user_embedding_common(const std::string &current_rank_time_path, const std::string &user_embedding_id, const std::string &user_embedding_file_name)
{
    std::string user_embedding_file_path = current_rank_time_path + "/" + user_embedding_file_name;
    if (!std::filesystem::exists(user_embedding_file_path))
    {
        std::optional<RecommendTraceUserEmbedding> recommend_trace_user_embedding = knowledgebase::findRecommendTraceUserEmbeddingByUniqueId(user_embedding_id);
        if (recommend_trace_user_embedding != std::nullopt)
        {
            RecommendTraceUserEmbedding current_recommend_trace_user_embedding = recommend_trace_user_embedding.value();
            // std::ofstream user_embedding_file(user_embedding_file_path);
            std::optional<web::json::value> current_value_optional = knowledgebase::convertFromRecommendTraceUserEmbeddingToWebJsonValue(current_recommend_trace_user_embedding);
            if (current_value_optional != std::nullopt)
            {
                web::json::value current_value = current_value_optional.value();
                std::vector<int> impression_integer_id_vec = current_recommend_trace_user_embedding.getImpressionIdUsedToCalculateEmbeddingVec();
                web::json::value entry_list = web::json::value::array();
                int index = 0;
                for (int current_impression_integer_id : impression_integer_id_vec)
                {
                    std::optional<Entry> current_entry_optional = getEntryByImpressionIntegerId(current_impression_integer_id);
                    if (current_entry_optional != std::nullopt)
                    {
                        Entry current_entry = current_entry_optional.value();
                        web::json::value current_entry_json;
                        current_entry_json["integer_id"] = web::json::value::number(int(current_entry.integer_id));
                        current_entry_json["title"] = web::json::value::string(current_entry.title);
                        current_entry_json["url"] = web::json::value::string(current_entry.url);
                        entry_list[index++] = current_entry_json;
                    }
                }
                current_value["entry_list"] = entry_list;
                writeWebJsonValueToFile(current_value, user_embedding_file_path);
                // current_value.serialize(user_embedding_file);
            }
            // user_embedding_file.close();
        }
    }
    else
    {
        LOG(DEBUG) << "user_embedding_file_path [" << user_embedding_file_path << "] already exist" << std::endl;
    }
}

void dump_traceinfo_main(std::string source_name)
{
    knowledgebase::init_global_terminus_recommend_params();
    // get all rank_time
    std::vector<int> rank_time_list = knowledgebase::findAllRecomendTraceInfoRankTimesBySource(source_name);

    std::string recommend_trace_root(RECOMMEND_TRACE_ROOT);
    std::string recommend_trace_root_source = recommend_trace_root + "/" + source_name;
    if (!std::filesystem::exists(recommend_trace_root_source))
    {
        LOG(DEBUG) << "create recommend_trace_root_source " << recommend_trace_root_source << std::endl;
        std::filesystem::create_directories(recommend_trace_root_source);
    }

    std::vector<std::string> list_need_zip_directory;
    for (int index = 0; index < std::min(int(rank_time_list.size()), int(globalTerminusRecommendParams.trace_info_number_zip)); index++)
    {
        int rank_time = rank_time_list[index];
        std::optional<RecommendTraceInfo> current_trace_info = knowledgebase::findRecommendTraceInfoByRankTimeAndSource(source_name, rank_time);
        if (current_trace_info != std::nullopt)
        {
            RecommendTraceInfo current_trace_info_value = current_trace_info.value();
            std::string current_rank_time = std::to_string(rank_time);
            std::string current_rank_time_path = recommend_trace_root_source + "/" + current_rank_time;
            if (!std::filesystem::exists(current_rank_time_path))
            {
                LOG(DEBUG) << "create current_rank_time_path " << current_rank_time_path << std::endl;
                std::filesystem::create_directories(current_rank_time_path);
            }

            write_not_impressioned_entry(current_rank_time_path, current_trace_info_value);
            write_added_not_impressioned_entry(current_rank_time_path, current_trace_info_value);
            write_impressioned_clicked_entry(current_rank_time_path, current_trace_info_value);
            write_added_impressioned_clicked_entry(current_rank_time_path, current_trace_info_value);
            write_top_ranked_entry(current_rank_time_path, current_trace_info_value);
            write_recommend_parameter(current_rank_time_path, current_trace_info_value);
            write_user_embedding_common(current_rank_time_path, current_trace_info_value.long_term_user_embedding_id, LONGE_TERM_USER_EMBEDDING_FILE_NAME);
            write_user_embedding_common(current_rank_time_path, current_trace_info_value.short_term_user_embedding_id, SHORT_TERM_USER_EMBEDDING_FILE_NAME);
            list_need_zip_directory.push_back(current_rank_time_path);
        }
    }

    if (list_need_zip_directory.size() > globalTerminusRecommendParams.trace_info_number_zip)
    {
        for (int index = globalTerminusRecommendParams.trace_info_number_zip; index < list_need_zip_directory.size(); index++)
        {
            std::string current_rank_time_path = list_need_zip_directory[index];
            std::filesystem::remove_all(current_rank_time_path);
        }
    }

    if (list_need_zip_directory.size() > 0)
    {
        std::string tar_gz_file_path = recommend_trace_root + "/" + source_name + ".tar.gz";
        create_tar_gz(recommend_trace_root_source, tar_gz_file_path);
    }
}