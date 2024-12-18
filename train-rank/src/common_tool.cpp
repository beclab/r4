#include <chrono>
#include <iostream>
#include <vector>
#include <eigen3/Eigen/Dense>
using namespace Eigen;

// using namespace std;

#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

void init_log()
{
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

void printVector(const std::vector<std::string> &a)
{
  std::cout << "The vector elements are : ";

  for (int i = 0; i < a.size(); i++)
    std::cout << a.at(i) << ' ';
}

bool isStringEmptyOrWhitespace(const std::string &str)
{
  // Check if the string is empty
  if (str.empty())
  {
    return true;
  }
  // Check if all characters in the string are whitespace
  return std::all_of(str.begin(), str.end(),
                     [](unsigned char ch)
                     { return std::isspace(ch); });
}

int countStringToken(const std::string &content)
{
  std::vector<std::string> tokens;

  // stringstream class check1
  std::stringstream check1(content);

  std::string intermediate;

  // Tokenizing w.r.t. space ' '
  while (getline(check1, intermediate, ' '))
  {
    if (isStringEmptyOrWhitespace(intermediate))
    {
      continue;
    }
    tokens.push_back(intermediate);
  }
  return tokens.size();
}

bool isConvertibleToInt(const std::string &str)
{
  bool result = false;
  try
  {
    std::stoi(str);
    result = true;
  }
  catch (...)
  {
  }
  return result;
}

std::time_t getTimeStampNow()
{
  std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> tp =
      std::chrono::time_point_cast<std::chrono::seconds>(
          std::chrono::system_clock::now());
  std::time_t timestamp = tp.time_since_epoch().count();
  return timestamp;
}

std::string envOrBlank(const char *env)
{
  auto envvar = std::getenv(env);
  if (envvar == nullptr)
  {
    return "";
  }
  else
  {
    return std::string(envvar);
  }
}

int getEnvInt(const char *envVar, int defaultValue)
{
  // Get the environment variable
  const char *envValue = std::getenv(envVar);

  // If the environment variable does not exist, return the default value
  if (envValue == nullptr)
  {
    return defaultValue;
  }

  // Try to convert the environment variable value to an integer
  try
  {
    return std::stoi(envValue); // Use std::stoi to convert the string to an integer
  }
  catch (const std::invalid_argument &e)
  {
    // If the conversion fails (e.g., the environment variable value cannot be converted to an integer), return the default value
    std::cerr << "Error: Invalid integer in environment variable '" << envVar << "'." << std::endl;
  }
  catch (const std::out_of_range &e)
  {
    // If the integer overflows, return the default value
    std::cerr << "Error: Integer overflow in environment variable '" << envVar << "'." << std::endl;
  }

  // If an exception occurs, return the default value
  return defaultValue;
}

void calculate_embedding()
{
  int n = 1000; // 假设我们有 1000 个 embedding，每个是大小为 128 的 vector
  int embedding_dim = 128;

  // 创建一个 Eigen 矩阵来存储这些向量
  MatrixXf embeddings(n, embedding_dim);

  // 填充这些向量
  for (int i = 0; i < n; ++i)
  {
    for (int j = 0; j < embedding_dim; ++j)
    {
      embeddings(i, j) = static_cast<float>(i + j);
    }
  }

  // 将 n 个向量相加
  VectorXf result = embeddings.colwise().sum();

  // 输出结果
  std::cout << "Sum of embeddings: " << result.transpose() << std::endl;
}