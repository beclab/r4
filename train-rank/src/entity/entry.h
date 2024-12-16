#pragma once

#include <optional>
#include <string>
#include <vector>

#include <boost/date_time.hpp>

static const char ENTRY_ID[] = "id";
static const char ENTRY_FILE_TYPE[] = "file_type";
static const char ENTRY_READ_LATER[] = "readlater";
static const char ENTRY_CRAWLER[] = "crawler";
static const char ENTRY_STARRED[] = "starred";
static const char ENTRY_DISABLED[] = "disabled";
static const char ENTRY_SAVED[] = "saved";
static const char ENTRY_UNREAD[] = "unread";
static const char ENTRY_EXTRACT[] = "extract";
static const char ENTRY_LANGUAGE[] = "language";
static const char ENTRY_URL[] = "url";
static const char ENTRY_PURE_CONTENT[] = "pure_content";
static const char ENTRY_TITLE[] = "title";
static const char ENTRY_CREATED_AT[] = "created_at";
static const char ENTRY_LAST_OPENED[] = "last_opened";

struct Entry
{
  std::string id;
  std::string file_type;
  bool readlater;
  bool crawler;
  bool starred;
  bool disabled;
  bool saved;
  bool unread;
  bool extract;
  std::string language;
  std::string url;
  std::string pure_content;
  std::string title;
  boost::posix_time::ptime timestamp;
  long long last_opened;
};
