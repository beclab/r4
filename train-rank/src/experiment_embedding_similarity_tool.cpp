#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>
#include <xlnt/xlnt.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>
#include <filesystem>
#include <unordered_set>

#include "faiss_article_search.h"
#include "userembedding_calculation.h"

struct SimpleEntry
{
    std::string id;
    std::string url;
    std::string title;
    int created_at_timestamp;
    int published_at_timestamp;
};

namespace fs = std::filesystem;

void createDirectoryIfNotExists(const std::string &path)
{
    if (!fs::exists(path))
    {
        if (fs::create_directory(path))
        {
            std::cout << "Directory created: " << path << std::endl;
        }
        else
        {
            std::cerr << "Failed to create directory: " << path << std::endl;
        }
    }
    else
    {
        std::cout << "Directory already exists: " << path << std::endl;
    }
}

std::unordered_map<std::string, std::string> getAllUniqueEntries(SQLite::Database &db)
{
    std::unordered_map<std::string, std::string> id_to_title;
    std::unordered_map<std::string, std::string> title_to_id;
    SQLite::Statement query(db, "SELECT DISTINCT title, id FROM entries");

    while (query.executeStep())
    {
        std::string title = query.getColumn(0);
        std::string id = query.getColumn(1);
        // id_to_title[id] = title;
        title_to_id[title] = id;
    }
    for (const auto &entry : title_to_id)
    {
        id_to_title[entry.second] = entry.first;
    }
    std::cout << "Number of unique entries: " << id_to_title.size() << std::endl;
    std::cout << "Number of unique titles: " << title_to_id.size() << std::endl;

    return id_to_title;
}

std::vector<float> stringToFloatVector(const std::string &str)
{
    std::vector<float> result;
    std::string trimmed = str.substr(1, str.size() - 2); // 去掉开头和结尾的方括号
    std::istringstream ss(trimmed);
    std::string item;

    while (std::getline(ss, item, ';'))
    {
        result.push_back(std::stof(item));
    }

    return result;
}

std::ostream &operator<<(std::ostream &os, const std::vector<float> &vec)
{
    std::cout << "vec.size() = " << vec.size() << std::endl;
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        os << vec[i];
        if (i != vec.size() - 1)
        {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

std::unordered_map<std::string, std::vector<float>> getEmbeddingsFromDatabase(const std::vector<std::string> &ids, const std::string &db_filename)
{
    SQLite::Database db(db_filename, SQLite::OPEN_READONLY);
    std::unordered_map<std::string, std::vector<float>> embeddings;

    SQLite::Statement query(db, "SELECT id, embedding FROM entries WHERE id = ?");

    for (const auto &id : ids)
    {
        query.bind(1, id);

        if (query.executeStep())
        {
            std::string tid = query.getColumn(0).getText();
            std::string embedding = query.getColumn(1).getText();
            std::vector<float> embedding_vector = stringToFloatVector(embedding);
            // std::cout << embedding << std::endl;
            // std::cout << embedding_vector << std::endl;
            embeddings[tid] = embedding_vector;
        }

        query.reset();
    }

    return embeddings;
}

std::vector<std::string> getRandomEntryIds(const std::unordered_map<std::string, std::string> &entries, int number)
{
    std::vector<std::string> ids;
    for (const auto &entry : entries)
    {
        ids.push_back(entry.first);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ids.begin(), ids.end(), g);

    if (number < ids.size())
    {
        ids.resize(number);
    }

    return ids;
}

std::vector<SimpleEntry> getEntriesByIds(SQLite::Database &db, const std::vector<std::string> &ids)
{
    std::vector<SimpleEntry> entries;
    SQLite::Statement query(db, "SELECT id, url, title, created_at_timestamp, published_at_timestamp FROM entries WHERE id = ?");

    for (const auto &id : ids)
    {
        query.bind(1, id);
        if (query.executeStep())
        {
            SimpleEntry entry = {
                query.getColumn(0),
                query.getColumn(1),
                query.getColumn(2),
                query.getColumn(3),
                query.getColumn(4)};
            entries.push_back(entry);
        }
        query.reset();
    }

    return entries;
}

void writeEntriesToExcel(const std::vector<SimpleEntry> &entries, const std::string &filename)
{
    xlnt::workbook wb;
    xlnt::worksheet ws = wb.active_sheet();

    // Write header row
    ws.cell("A1").value("ID");
    ws.cell("B1").value("URL");
    ws.cell("C1").value("Title");
    ws.cell("D1").value("Created At");
    ws.cell("E1").value("Published At");

    // Write data rows
    int row = 2;
    for (const auto &entry : entries)
    {
        ws.cell("A" + std::to_string(row)).value(entry.id);
        ws.cell("B" + std::to_string(row)).value(entry.url);
        ws.cell("C" + std::to_string(row)).value(entry.title);
        ws.cell("D" + std::to_string(row)).value(entry.created_at_timestamp);
        ws.cell("E" + std::to_string(row)).value(entry.published_at_timestamp);
        row++;
    }

    wb.save(filename);
}

std::unordered_map<std::string, SimpleEntry> readBaseEntriesFromExcel(const std::string &filename)
{
    xlnt::workbook wb;
    wb.load(filename);
    xlnt::worksheet ws = wb.active_sheet();

    std::vector<SimpleEntry> entries;
    std::unordered_map<std::string, SimpleEntry> entries_map;

    // Start reading from the second row (assuming the first row is the header)
    for (auto row : ws.rows(false))
    {
        if (row[0].row() == 1) // Skip header row
        {
            continue;
        }

        SimpleEntry entry;
        entry.id = row[0].to_string();
        entry.url = row[1].to_string();
        entry.title = row[2].to_string();
        entry.created_at_timestamp = std::stoi(row[3].to_string());
        entry.published_at_timestamp = std::stoi(row[4].to_string());

        entries.push_back(entry);
    }

    for (const auto &entry : entries)
    {
        entries_map[entry.id] = entry;
    }

    return entries_map;
}

void createBaseEntry(const std::string &sqlite_db, int number_entry, const std::string &output_excel_path)
{
    try
    {
        // Open database
        SQLite::Database db(sqlite_db, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        // Get all unique entries by title
        auto uniqueEntries = getAllUniqueEntries(db);

        // Get specified number of random entry IDs
        auto selectedIds = getRandomEntryIds(uniqueEntries, number_entry);

        // Get other information of entries by IDs
        auto entries = getEntriesByIds(db, selectedIds);

        // Write to Excel file
        writeEntriesToExcel(entries, output_excel_path);

        std::cout << "Data has been successfully written to entries.xlsx" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

std::pair<std::vector<std::string>, std::vector<std::string>> splitRandomly(const std::vector<std::string> &input, int number)
{
    std::vector<std::string> selected;
    std::vector<std::string> remaining = input;

    if (input.size() <= number)
    {
        return std::make_pair(input, std::vector<std::string>{});
    }

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(remaining.begin(), remaining.end(), g);

    selected.insert(selected.end(), remaining.begin(), remaining.begin() + number);
    remaining.erase(remaining.begin(), remaining.begin() + number);

    return std::make_pair(selected, remaining);
}

template <typename T>
std::unordered_set<T> intersection(const std::unordered_set<T> &set1, const std::unordered_set<T> &set2)
{
    std::unordered_set<T> result;
    for (const auto &elem : set1)
    {
        if (set2.find(elem) != set2.end())
        {
            result.insert(elem);
        }
    }
    return result;
}

void writeEachEntrySimilarEntriesToExcel(const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> &each_entry_to_similar_entries,
                                         const std::unordered_map<std::string, SimpleEntry> &current_entries, const std::string &output_excel_path)
{
    xlnt::workbook wb;
    xlnt::worksheet ws = wb.active_sheet();

    // Write header row
    ws.cell("A1").value("ID");
    ws.cell("B1").value("URL");
    ws.cell("C1").value("Title");
    ws.cell("D1").value("FirstSimilarTitle");
    ws.cell("E1").value("SecondSimilarTitle");
    ws.cell("F1").value("ThirdSimilarTitle");

    // Write data rows
    int row = 2;
    for (const auto &entry : each_entry_to_similar_entries)
    {
        ws.cell("A" + std::to_string(row)).value(entry.first);
        ws.cell("B" + std::to_string(row)).value(current_entries.at(entry.first).url);
        ws.cell("C" + std::to_string(row)).value(current_entries.at(entry.first).title);

        if (entry.second.size() >= 1)
        {
            ws.cell("D" + std::to_string(row)).value(current_entries.at(entry.second[0].first).title);
        }
        if (entry.second.size() >= 2)
        {
            ws.cell("E" + std::to_string(row)).value(current_entries.at(entry.second[1].first).title);
        }
        if (entry.second.size() >= 3)
        {
            ws.cell("F" + std::to_string(row)).value(current_entries.at(entry.second[2].first).title);
        }

        row++;
    }

    wb.save(output_excel_path);
}

void experimentOneTime(const std::string &experiment_base_entry_path, const std::string &sqlitedb_path, const std::string &expriment_directory)
{

    std::unordered_set<std::string> all_title;
    std::unordered_map<std::string, SimpleEntry> current_entries = readBaseEntriesFromExcel(experiment_base_entry_path);
    for (const auto &entry : current_entries)
    {
        all_title.insert(entry.second.title);
    }
    std::cout << "All title: " << all_title.size() << std::endl;
    std::vector<std::string> ids;
    for (const auto &entry : current_entries)
    {
        ids.push_back(entry.first);
    }
    std::unordered_map<std::string, std::vector<float>> id_to_embedding = getEmbeddingsFromDatabase(ids, sqlitedb_path);
    int evaluate_number = 50;
    int one_entry_most_similar_number = 3;
    std::pair<std::vector<std::string>, std::vector<std::string>> result = splitRandomly(ids, evaluate_number);
    std::unordered_set<std::string> searched_title;
    std::unordered_set<std::string> searched_ids;
    std::unordered_set<std::string> target_title;
    std::unordered_set<std::string> target_ids;
    for (const auto &id : result.first)
    {
        searched_title.insert(current_entries[id].title);
        searched_ids.insert(id);
    }
    for (const auto &id : result.second)
    {
        target_title.insert(current_entries[id].title);
        target_ids.insert(id);
    }
    std::cout << "Searched: " << searched_title.size() << " Target: " << target_title.size() << std::endl;
    auto intersection_title = intersection(searched_title, target_title);
    std::cout << "Intersection: " << intersection_title.size() << std::endl;
    std::cout << "Searched IDs: " << searched_ids.size() << " Target IDs: " << target_ids.size() << std::endl;
    std::unordered_map<int, std::string> row_index_to_entry_id;
    std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> similar_entries_widh_ids_map; // Randomly selected 50 entries' similar entries
    std::vector<std::pair<std::string, float>> similar_entries_widh_ids;
    std::vector<std::pair<std::string, float>> user_embedding_most_similar_entries_with_ids;

    std::unordered_set<std::string> each_entry_title;
    std::unordered_set<std::string> merged_entry_title;
    {
        std::vector<std::vector<float>> embedding_list;
        int index = 0;
        for (const auto &id : result.second)
        {
            embedding_list.push_back(id_to_embedding[id]);
            row_index_to_entry_id[index++] = id;
        }
        FAISSArticleSearch search(embedding_list);
        std::vector<std::vector<float>> candidate_user_embedding_list;

        for (const auto &id : result.first)
        {
            std::cout << "target title: " << current_entries[id].title << std::endl;
            std::vector<std::pair<int, float>> similar_entries = search.findMostSimilarArticles(id_to_embedding[id], one_entry_most_similar_number);

            std::vector<std::pair<std::string, float>> similar_entries_widh_ids;
            for (const auto &entry : similar_entries)
            {
                similar_entries_widh_ids.push_back(std::make_pair(row_index_to_entry_id[entry.first], entry.second));
            }
            for (const auto &entry : similar_entries_widh_ids)
            {
                each_entry_title.insert(current_entries[entry.first].title);
                std::cout << "similar title: " << current_entries[entry.first].title << " distance: " << entry.second << std::endl;
            }
            similar_entries_widh_ids_map[id] = similar_entries_widh_ids;
            candidate_user_embedding_list.push_back(id_to_embedding[id]);
            std::cout << "====================================" << std::endl;
        }
        std::vector<float> user_embedding = calcluateUserEmbeddingSimple(candidate_user_embedding_list);
        std::vector<std::pair<int, float>> user_embedding_most_similar_entries = search.findMostSimilarArticles(user_embedding, one_entry_most_similar_number * evaluate_number);
        for (const auto &entry : user_embedding_most_similar_entries)
        {
            user_embedding_most_similar_entries_with_ids.push_back(std::make_pair(row_index_to_entry_id[entry.first], entry.second));
        }
        for (const auto &entry : user_embedding_most_similar_entries_with_ids)
        {
            merged_entry_title.insert(current_entries[entry.first].title);
            std::cout << "user embedding similar title: " << current_entries[entry.first].title << " distance: " << entry.second << std::endl;
        }
    }
    std::cout << "Each entry title: " << each_entry_title.size() << " Merged entry title: " << merged_entry_title.size() << std::endl;
    auto searched_intersection_title = intersection(each_entry_title, merged_entry_title);
    std::cout << "Intersection title: " << searched_intersection_title.size() << std::endl;
    std::cout << "Selected: " << result.first.size() << " Remaining: " << result.second.size() << std::endl;

    // 50 entries' most similar 3 entries
    writeEachEntrySimilarEntriesToExcel(similar_entries_widh_ids_map, current_entries, expriment_directory + "/each_entry_similar_entries.xlsx");

    // User embedding's most similar 150 entries
    std::vector<SimpleEntry> user_embedding_most_similar_simple_entry;
    for (const auto &entry : user_embedding_most_similar_entries_with_ids)
    {
        user_embedding_most_similar_simple_entry.push_back(current_entries[entry.first]);
    }
    writeEntriesToExcel(user_embedding_most_similar_simple_entry, expriment_directory + "/user_embedding_most_similar_entries.xlsx");
}

int main()
{

    std::string experiment_base_entry_path = "/opt/rss-termius-v2-rank/base100000.xlsx";
    std::string sqlitedb_path = "/opt/rss-termius-v2-rank/zhcn.db";
    for (int i = 1; i < 11; i++)
    {
        std::string experiment_directory = "/opt/rss-termius-v2-rank/experiment/experiment_" + std::to_string(i);
        createDirectoryIfNotExists(experiment_directory);
        experimentOneTime(experiment_base_entry_path, sqlitedb_path, experiment_directory);
    }
    // std::unordered_map<std::string, SimpleEntry> current_entries = readBaseEntriesFromExcel(experiment_base_entry_path);
    // std::cout << "current_entries size: " << current_entries.size() << std::endl;

    return 0;
}