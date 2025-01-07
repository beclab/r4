#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>
#include <xlnt/xlnt.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>

struct SimpleEntry
{
    std::string id;
    std::string url;
    std::string title;
    int created_at_timestamp;
    int published_at_timestamp;
};

std::unordered_map<std::string, std::string> getAllUniqueEntries(SQLite::Database &db)
{
    std::unordered_map<std::string, std::string> entries;
    SQLite::Statement query(db, "SELECT DISTINCT title, id FROM entries");

    while (query.executeStep())
    {
        std::string title = query.getColumn(0);
        std::string id = query.getColumn(1);
        entries[id] = title;
    }

    return entries;
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

void experimentOneTime(const std::string &experiment_base_entry_path, const std::string &sqlitedb_path, const std::string &expriment_directory)
{
}

int main()
{
    // createBaseEntry("/opt/rss-termius-v2-rank/zhcn.db", 100000, "/opt/rss-termius-v2-rank/base100000.xlsx");
}