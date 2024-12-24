#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <eigen3/Eigen/Dense>
#include <random>
#include <openssl/sha.h>

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

int getEnvFloat(const char *envVar, float defaultValue)
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
    return std::stof(envValue); // Use std::stoi to convert the string to an integer
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
  int n = 1000; // Assume we have 1000 embeddings, each is a vector of size 128
  int embedding_dim = 128;

  // Create an Eigen matrix to store these vectors
  MatrixXf embeddings(n, embedding_dim);

  // Fill these vectors
  for (int i = 0; i < n; ++i)
  {
    for (int j = 0; j < embedding_dim; ++j)
    {
      embeddings(i, j) = static_cast<float>(i + j);
    }
  }

  // Sum the n vectors
  VectorXf result = embeddings.colwise().sum();

  // Output the result
  std::cout << "Sum of embeddings: " << result.transpose() << std::endl;
}

bool are_equal_double(double a, double b, double epsilon = 1e-5)
{
  return std::fabs(a - b) < epsilon;
}

double eigen_cosine_similarity(const VectorXd &A, const VectorXd &B)
{
  // Calculate dot product
  double dot_product = A.dot(B);

  // Calculate magnitudes
  double magnitude_A = A.norm();
  double magnitude_B = B.norm();

  // Calculate cosine similarity
  return dot_product / (magnitude_A * magnitude_B);
}

VectorXd vectorToEigentVectorXd(const std::vector<double> &vec)
{
  return Eigen::Map<const VectorXd>(vec.data(), vec.size());
}

double normalized_similarity_score_based_on_cosine_similarity(const VectorXd &A, const VectorXd &B)
{
  double cosine_similarity = eigen_cosine_similarity(A, B);
  return (cosine_similarity + 1) / 2;
}

float randomFloatBetweenZeroAndOne()
{
  // Create a random number generator (uniform distribution between 0 and 1)
  std::random_device rd;
  std::mt19937 gen(rd());                         // Mersenne Twister random number generator
  std::uniform_real_distribution<> dis(0.0, 1.0); // Uniform distribution between 0 and 1

  // Generate a random number between 0 and 1
  return dis(gen);
}

std::string generateSHA256Hash(const std::vector<double> &vec, const std::string &impression_id)
{
  // Convert vector<double> to string
  std::string combined_str;
  for (const double &val : vec)
  {
    combined_str += std::to_string(val) + ",";
  }
  combined_str += impression_id;

  // Use OpenSSL to compute SHA-256 hash
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, combined_str.c_str(), combined_str.size());
  SHA256_Final(hash, &sha256);

  // Convert hash value to hexadecimal string
  std::string hash_hex;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
  {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", hash[i]);
    hash_hex += buf;
  }

  return hash_hex;
}

std::string arrayToString(const std::vector<int> &arr)
{
  if (arr.empty())
    return "";

  std::ostringstream oss;
  int start = arr[0], end = arr[0];

  for (size_t i = 1; i < arr.size(); ++i)
  {
    if (arr[i] == end + 1)
    {
      end = arr[i];
    }
    else
    {
      if (start == end)
      {
        oss << start << ";";
      }
      else
      {
        oss << start << "-" << end << ";";
      }
      start = end = arr[i];
    }
  }

  if (start == end)
  {
    oss << start;
  }
  else
  {
    oss << start << "-" << end;
  }

  return oss.str();
}

std::vector<int> stringToArray(const std::string &str)
{
  std::vector<int> result;
  std::istringstream iss(str);
  std::string token;

  while (std::getline(iss, token, ';'))
  {
    size_t dashPos = token.find('-');
    if (dashPos == std::string::npos)
    {
      result.push_back(std::stoi(token));
    }
    else
    {
      int start = std::stoi(token.substr(0, dashPos));
      int end = std::stoi(token.substr(dashPos + 1));
      for (int i = start; i <= end; ++i)
      {
        result.push_back(i);
      }
    }
  }

  return result;
}