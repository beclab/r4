#pragma once
#include "entity/entry.h"
static const std::string NOT_IMRESSIONED_FILE_NAME = "not_impressioned.xlsx";
static const std::string ADDED_NOT_IMRESSIONED_FILE_NAME = "added_not_impressioned.xlsx";
static const std::string IMPRESSIONED_CLICKED_FILE_NAME = "impressioned_clicked.xlsx";
static const std::string ADDED_IMPRESSIONED_CLICKED_FILE_NAME = "added_impressioned_clicked.xlsx";
static const std::string TOP_RANKED_ENTRY_FILE_NAME = "top_ranked_entry.xlsx";
void dump_traceinfo_main(std::string source_name);
void writeEntryToExcel(const std::vector<Entry> &entries, const std::string &xlsx_path);
void writeEntryToExcelWithScore(const std::vector<Entry> &entries, const std::string &xlsx_path, const vector<double> &score);
