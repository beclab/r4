#include <iostream>

#include <filesystem>
#include <fstream>
#include <random>
#include <set>
#include <SQLiteCpp/SQLiteCpp.h>
#include <xlnt/xlnt.hpp>

#include "../../src/json.hpp"
#include "../../src/faiss_article_search.h"

namespace fs = std::filesystem;

struct EmbeddingStorage
{
    std::string id;
    std::vector<float> embedding;
};

struct Entry
{
    std::string id;
    std::string title;
    std::string url;
    std::string summary;
};

void writeToExcel(const std::vector<Entry> &entries, const std::string &filePath)
{
    xlnt::workbook wb;                      // Create workbook
    xlnt::worksheet ws = wb.active_sheet(); // Get active worksheet

    // Set headers
    ws.cell("A1").value("ID");
    ws.cell("B1").value("Title");
    ws.cell("C1").value("URL");
    ws.cell("D1").value("Summary");

    // Fill data from vector
    int row = 2; // Start writing data from the second row
    for (const auto &entry : entries)
    {
        ws.cell("A" + std::to_string(row)).value(entry.id);
        ws.cell("B" + std::to_string(row)).value(entry.title);
        ws.cell("C" + std::to_string(row)).value(entry.url);
        ws.cell("D" + std::to_string(row)).value(entry.summary);

        row++; // Process next row
    }

    // Save to file
    wb.save(filePath);
}
EmbeddingStorage parseEmbeddingStorage(const std::string &json_path);

std::vector<fs::directory_entry> listDirectory(const fs::path &dir)
{
    std::vector<fs::directory_entry> entries;
    try
    {
        if (!fs::exists(dir))
        {
            std::cerr << "Directory does not exist: " << dir << std::endl;
            return entries;
        }

        if (!fs::is_directory(dir))
        {
            std::cerr << "Path is not a directory: " << dir << std::endl;
            return entries;
        }

        std::cout << "Files in directory: " << dir << std::endl;

        for (const auto &entry : fs::directory_iterator(dir))
        {
            if (entry.is_regular_file())
            {
                entries.push_back(entry);
                std::cout << "File: " << entry.path() << std::endl;
            }
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return entries;
}

std::map<std::string, std::vector<float>> read_summary_embeddings(std::string directory)
{
    std::map<std::string, std::vector<float>> summary_embeddings;
    std::vector<fs::directory_entry> allL_entries = listDirectory(directory);
    for (auto &entry : allL_entries)
    {
        std::string file_path = entry.path().string();
        EmbeddingStorage current = parseEmbeddingStorage(file_path);
        summary_embeddings[current.id] = current.embedding;
    }
    return summary_embeddings;
}

EmbeddingStorage parseEmbeddingStorage(const std::string &json_path)
{
    EmbeddingStorage data;
    try
    {
        std::ifstream file(json_path);
        nlohmann::json json_value;
        file >> json_value;
        // Parse id field
        if (json_value.contains("id") && json_value["id"].is_string())
        {
            data.id = json_value["id"].get<std::string>();
        }
        else
        {
            std::cerr << "Error: Missing or invalid 'id' field in JSON." << std::endl;
        }

        // Parse embedding field
        if (json_value.contains("embedding") && json_value["embedding"].is_array())
        {
            data.embedding = json_value["embedding"].get<std::vector<float>>();
        }
        else
        {
            std::cerr << "Error: Missing or invalid 'embedding' field in JSON." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
    return data;
}

std::vector<std::string> extractRandomKeys(const std::map<std::string, std::vector<float>> &myMap, int numKeys = 50)
{
    // Extract all keys into a vector
    std::vector<std::string> keys;
    for (const auto &pair : myMap)
    {
        keys.push_back(pair.first);
    }

    // If the number of keys is less than 50, return all keys directly
    if (keys.size() <= numKeys)
    {
        return keys;
    }

    // Shuffle the order of keys
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(keys.begin(), keys.end(), g);

    // Return the first 50 keys
    keys.resize(numKeys);
    return keys;
}

struct SingleSimilarityItem
{
    std::string target_id;
    std::string target_title;
    std::string target_url;
    std::string target_summary;

    std::string first_id;
    std::string first_title;
    std::string first_url;
    std::string first_summary;

    std::string second_id;
    std::string second_title;
    std::string second_url;
    std::string second_summary;

    std::string third_id;
    std::string third_title;
    std::string third_url;
    std::string third_summary;
};

struct SummaryStorage
{
    std::string summary;
    std::vector<std::string> keywords;
    std::vector<std::string> categories;
};

SummaryStorage parseSummaryStorageFromJson(const std::string &jsonFilePath)
{
    // Read JSON file
    std::ifstream file(jsonFilePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open the file: " + jsonFilePath);
    }

    // Parse JSON data
    nlohmann::json jsonData;
    file >> jsonData;

    // Create SummaryStorage object and fill data
    SummaryStorage storage;

    // Extract data from JSON
    storage.summary = jsonData["summary"].get<std::string>();

    // Extract categories and keywords
    for (const auto &category : jsonData["categories"])
    {
        storage.categories.push_back(category.get<std::string>());
    }

    for (const auto &keyword : jsonData["keywords"])
    {
        storage.keywords.push_back(keyword.get<std::string>());
    }

    return storage;
}
// Read all entries from SQLite database and store them in a map
std::map<std::string, Entry>
getEntriesFromDatabase(const std::string &dbPath)
{
    std::map<std::string, Entry> entries;

    try
    {
        // Open SQLite database
        SQLite::Database db(dbPath, SQLite::OPEN_READWRITE);

        // Execute SQL query, only select id, title, url fields
        SQLite::Statement query(db, "SELECT id, title, url FROM entries");

        // Iterate through query results
        while (query.executeStep())
        {
            // Extract fields from query result
            Entry entry;
            entry.id = query.getColumn(0).getText();    // id
            entry.title = query.getColumn(1).getText(); // title
            entry.url = query.getColumn(2).getText();   // url
            // std::cout << entry.id << " " << entry.title << " " << entry.url << std::endl;
            // Insert data into map
            entries[entry.id] = entry;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error while reading database: " << e.what() << std::endl;
    }

    return entries;
}

// Write vector<SingleSimilarityItem> to Excel file
void writeToExcel(const std::vector<SingleSimilarityItem> &items, const std::string &filePath)
{
    xlnt::workbook wb;                      // Create workbook
    xlnt::worksheet ws = wb.active_sheet(); // Get active worksheet

    // Set headers
    ws.cell("A1").value("Target ID");
    ws.cell("B1").value("Target Title");
    ws.cell("C1").value("Target URL");
    ws.cell("D1").value("Target Summary");

    ws.cell("E1").value("First ID");
    ws.cell("F1").value("First Title");
    ws.cell("G1").value("First URL");
    ws.cell("H1").value("First Summary");

    ws.cell("I1").value("Second ID");
    ws.cell("J1").value("Second Title");
    ws.cell("K1").value("Second URL");
    ws.cell("L1").value("Second Summary");

    ws.cell("M1").value("Third ID");
    ws.cell("N1").value("Third Title");
    ws.cell("O1").value("Third URL");
    ws.cell("P1").value("Third Summary");

    // Fill data from vector
    int row = 2; // Start writing data from the second row
    for (const auto &item : items)
    {
        ws.cell("A" + std::to_string(row)).value(item.target_id);
        ws.cell("B" + std::to_string(row)).value(item.target_title);
        ws.cell("C" + std::to_string(row)).value(item.target_url);
        ws.cell("D" + std::to_string(row)).value(item.target_summary);

        ws.cell("E" + std::to_string(row)).value(item.first_id);
        ws.cell("F" + std::to_string(row)).value(item.first_title);
        ws.cell("G" + std::to_string(row)).value(item.first_url);
        ws.cell("H" + std::to_string(row)).value(item.first_summary);

        ws.cell("I" + std::to_string(row)).value(item.second_id);
        ws.cell("J" + std::to_string(row)).value(item.second_title);
        ws.cell("K" + std::to_string(row)).value(item.second_url);
        ws.cell("L" + std::to_string(row)).value(item.second_summary);

        ws.cell("M" + std::to_string(row)).value(item.third_id);
        ws.cell("N" + std::to_string(row)).value(item.third_title);
        ws.cell("O" + std::to_string(row)).value(item.third_url);
        ws.cell("P" + std::to_string(row)).value(item.third_summary);

        row++; // Process next row
    }

    // Save to file
    wb.save(filePath);
}

int main()
{
    // The input is a sqlite db path, a folder path storing user summary embeddings, and an experiment output path

    // Read all summary embeddings from the path storing summary embeddings, store them in a map where the key is the summary id and the value is a vector<float> representing the summary embedding
    std::string embedding_directory = "/opt/data/process_result_gemi3_27b_zh_embedding";
    std::string db_path = "/opt/data/zhcn.db";
    std::string summary_directory = "/opt/data/process_result_gemi3_27b";
    std::string output_path = "/opt/data/experiment_3.xlsx";
    std::vector<std::string> queried_keys;
    FAISSArticleSearch *faiss_search = nullptr;
    std::map<int, std::string> row_index_to_id;
    std::map<std::string, int> id_to_row_index;
    std::map<std::string, std::vector<float>> queried_id_to_embedding;
    std::map<std::string, std::vector<std::string>> query_id_to_top3_gallery_ids;
    std::vector<std::string> exist_summary_ids;
    {
        std::map<std::string, std::vector<float>> id_to_embedding = read_summary_embeddings(embedding_directory);
        std::cout << "id_to_embedding size: " << id_to_embedding.size() << std::endl;
        // Randomly select 50 ids from all ids as query ids, and the rest as gallery ids
        queried_keys = extractRandomKeys(id_to_embedding);
        std::set<std::string> queried_keys_set(queried_keys.begin(), queried_keys.end());
        std::vector<std::vector<float>> embeddings;
        for (const auto &pair : id_to_embedding)
        {
            exist_summary_ids.push_back(pair.first);
            if (queried_keys_set.find(pair.first) == queried_keys_set.end())
            {
                row_index_to_id[row_index_to_id.size()] = pair.first;
                id_to_row_index[pair.first] = id_to_row_index.size();
                embeddings.push_back(pair.second);
            }
            else
            {
                queried_id_to_embedding[pair.first] = pair.second;
            }
        }
        // Load the embeddings of gallery ids into faiss search, construct faiss search with std::vector<std::vector<float>> &vectors, record the row_index corresponding to each id and the id corresponding to each row_index

        faiss_search = new FAISSArticleSearch(embeddings);
    }

    std::cout << "faiss search contructed" << std::endl;

    // For each query id, calculate the similarity between its embedding and all embeddings in the gallery, return the top 3 gallery ids
    for (const auto &pair : queried_id_to_embedding)
    {
        std::vector<std::pair<int, float>> top3 = faiss_search->findMostSimilarArticles(pair.second, 3);
        std::vector<std::string> top3_ids;
        for (const auto &item : top3)
        {
            top3_ids.push_back(row_index_to_id[item.first]);
        }
        query_id_to_top3_gallery_ids[pair.first] = top3_ids;
    }
    std::map<std::string, Entry> id_to_entries = getEntriesFromDatabase(db_path);
    std::cout << "id_to_entries size: " << id_to_entries.size() << std::endl;
    std::vector<SingleSimilarityItem> similarity_items;
    for (const auto &pair : query_id_to_top3_gallery_ids)
    {
        std::cout << "************************* current_id " << pair.first << " *************************" << std::endl;
        SingleSimilarityItem item;
        item.target_id = pair.first;
        item.target_title = id_to_entries[pair.first].title;
        item.target_url = id_to_entries[pair.first].url;
        std::string target_summary_path = summary_directory + "/" + pair.first;
        SummaryStorage storage = parseSummaryStorageFromJson(target_summary_path);
        item.target_summary = storage.summary;

        item.first_id = pair.second[0];
        item.first_title = id_to_entries[pair.second[0]].title;
        item.first_url = id_to_entries[pair.second[0]].url;
        std::string first_summary_path = summary_directory + "/" + pair.second[0];
        storage = parseSummaryStorageFromJson(first_summary_path);
        item.first_summary = storage.summary;

        item.second_id = pair.second[1];
        item.second_title = id_to_entries[pair.second[1]].title;
        item.second_url = id_to_entries[pair.second[1]].url;
        std::string second_summary_path = summary_directory + "/" + pair.second[1];
        storage = parseSummaryStorageFromJson(second_summary_path);
        item.second_summary = storage.summary;

        item.third_id = pair.second[2];
        item.third_title = id_to_entries[pair.second[2]].title;
        item.third_url = id_to_entries[pair.second[2]].url;
        std::string third_summary_path = summary_directory + "/" + pair.second[2];
        storage = parseSummaryStorageFromJson(third_summary_path);
        item.third_summary = storage.summary;
        similarity_items.push_back(item);
    }
    delete faiss_search;
    writeToExcel(similarity_items, output_path);
    // Save the final result in the form of an excel file
    std::cout << exist_summary_ids.size() << std::endl;

    std::vector<Entry> entries_summary;
    for (auto id : exist_summary_ids)
    {
        std::cout << "---------------------id: " << id << std::endl;

        Entry entry;
        entry.id = id;
        entry.title = id_to_entries[id].title;
        entry.url = id_to_entries[id].url;
        std::string summary_path = summary_directory + "/" + id;
        SummaryStorage storage = parseSummaryStorageFromJson(summary_path);
        entry.summary = storage.summary;
        entries_summary.push_back(entry);
    }
    writeToExcel(entries_summary, "/opt/data/entries.xlsx");
    std::cout << "hello world" << std::endl;
}