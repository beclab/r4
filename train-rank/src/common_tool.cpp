#include <chrono>
#include <iostream>
#include <vector>

// using namespace std;

#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

void init_log() {
  el::Configurations defaultConf;
  defaultConf.setToDefault();
  // Values are always std::string
  defaultConf.set(el::Level::Global, el::ConfigurationType::Format,
                  "[%datetime] [%file] [%line] [%func] %msg");
  // default logger uses default configurations
  el::Loggers::reconfigureLogger("default", defaultConf);
  LOG(INFO) << "Log using default file";
  // To set GLOBAL configurations you may use
}

void printVector(const std::vector<std::string>& a) {
  std::cout << "The vector elements are : ";

  for (int i = 0; i < a.size(); i++) std::cout << a.at(i) << ' ';
}

bool isStringEmptyOrWhitespace(const std::string& str) {
  // Check if the string is empty
  if (str.empty()) {
    return true;
  }
  // Check if all characters in the string are whitespace
  return std::all_of(str.begin(), str.end(),
                     [](unsigned char ch) { return std::isspace(ch); });
}

int countStringToken(const std::string& content) {
  std::vector<std::string> tokens;

  // stringstream class check1
  std::stringstream check1(content);

  std::string intermediate;

  // Tokenizing w.r.t. space ' '
  while (getline(check1, intermediate, ' ')) {
    if (isStringEmptyOrWhitespace(intermediate)) {
      continue;
    }
    tokens.push_back(intermediate);
  }
  return tokens.size();
}

bool isConvertibleToInt(const std::string& str) {
  bool result = false;
  try {
    std::stoi(str);
    result = true;
  } catch (...) {
  }
  return result;
}

std::time_t getTimeStampNow() {
  std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> tp =
      std::chrono::time_point_cast<std::chrono::seconds>(
          std::chrono::system_clock::now());
  std::time_t timestamp = tp.time_since_epoch().count();
  return timestamp;
}
