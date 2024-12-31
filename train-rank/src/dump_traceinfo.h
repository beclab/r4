#pragma once
#include "entity/entry.h"
#include <string>
#include <vector>
static const std::string NOT_IMRESSIONED_FILE_NAME = "not_impressioned.xlsx";
static const std::string ADDED_NOT_IMRESSIONED_FILE_NAME = "added_not_impressioned.xlsx";
static const std::string IMPRESSIONED_CLICKED_FILE_NAME = "impressioned_clicked.xlsx";
static const std::string ADDED_IMPRESSIONED_CLICKED_FILE_NAME = "added_impressioned_clicked.xlsx";
static const std::string TOP_RANKED_ENTRY_FILE_NAME = "top_ranked_entry.xlsx";
static const std::string PARAMETER_FILE_NAME = "parameter.json";
static const std::string LATEST_CLICKED_MOST_SIMILAR_THREE_FILE_NAME = "latest_clicked_most_similar.json";
static const std::string LONGE_TERM_USER_EMBEDDING_FILE_NAME = "long_term_user_embedding.json";
static const std::string SHORT_TERM_USER_EMBEDDING_FILE_NAME = "short_term_user_embedding.json";
static const std::string RECALL_USER_EMBEDDING_FILE_NAME = "recall_user_embedding.json";
void dump_traceinfo_main(std::string source_name);
void writeEntryToExcel(const std::vector<Entry> &entries, const std::string &xlsx_path);
void writeEntryToExcelWithScore(const std::vector<Entry> &entries, const std::string &xlsx_path, const std::vector<double> &score);
