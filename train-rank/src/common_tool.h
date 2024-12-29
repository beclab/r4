#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

// #include <eigen3/Eigen/Dense>
// using namespace Eigen;
using namespace std::chrono;

// DECLARE_string(model_path_root);

#include "easylogging++.h"
// using namespace std;
static const char TERMINUS_RECOMMEND_SOURCE_NAME[] =
    "TERMINUS_RECOMMEND_SOURCE_NAME";
static const char KNOWLEDGE_BASE_API_URL[] = "KNOWLEDGE_BASE_API_URL";
static const char TERMINUS_RECOMMEND_EMBEDDING_NAME[] =
    "EMBEDDING_METHOD"; // because user embedding module use "EMBEDDING_METHOD" as env name, so here also use it
static char RECOMMEND_MODEL_ROOT[] = "/opt/rank_model";
static char TERMINUS_RECOMMEND_EMBEDDING_DIMENSION[] =
    "TERMINUS_RECOMMEND_EMBEDDING_DIMENSION";

// user adjust parameter
static char TERMINUS_RECOMMEND_PARAMETER[] = "TERMINUS_RECOMMEND_PARAMETER";
// use how many impression to calculate short term user embedding, value > 0
static char TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION[] =
    "TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION"; // default 10

// use how many impression to calculate long term user embedding, value > 0
static char TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION[] =
    "TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION"; // default 10000

// The proportion of the cosine distance between the long-term user vector and the article when calculating
// the rank score, compared to the cosine distance between the short-term user vector and the article, value between 0 and 1
static char TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE[] =
    "TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE"; // default 0.3

//// The time weight of the article when calculating the rank score,
// compared to the cosine distance between the user vector and the article,value 0 and 1
static char TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE[] =
    "TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE"; // default 0.5

// If the number of clicked articles is less than or equal this value, do not use the recommendation algorithm.
//  Since they are all in one category, sort by time for cold start.
static char TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD[] =
    "TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD"; // default 10

// If the value is "long", use long-term user embedding as recall embedding,
// otherwise use short-term user embedding as recall embedding
static char TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING[] =
    "TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING"; // default "long"

// The number of zipped log
static char TERMINUS_RECOMMEND_TRACE_INFO_NUMBER[] = "TERMINUS_RECOMMEND_TRACE_INFO_NUMBER"; // default 100

void init_log();

void printVector(const std::vector<std::string> &a);

bool isStringEmptyOrWhitespace(const std::string &str);

int countStringToken(const std::string &content);

bool isConvertibleToInt(const std::string &str);

std::string envOrBlank(const char *env);

int getEnvInt(const char *envVar, int defaultValue);

int getEnvFloat(const char *envVar, float defaultValue);

std::string getEnvString(const char *envVar, const std::string &defaultValue);

template <class T1, class T2>
double AUROC(const T1 label[], const T2 score[], int n)
{
  for (int i = 0; i < n; i++)
    if (!std::isfinite(score[i]) || label[i] != 0 && label[i] != 1)
      return std::numeric_limits<double>::signaling_NaN();

  const auto order = new int[n];
  std::iota(order, order + n, 0);
  std::sort(order, order + n,
            [&](int a, int b)
            { return score[a] > score[b]; });
  const auto y = new double[n];
  const auto z = new double[n];
  for (int i = 0; i < n; i++)
  {
    y[i] = label[order[i]];
    z[i] = score[order[i]];
  }

  const auto tp = y; // Reuse
  std::partial_sum(y, y + n, tp);

  int top = 0; // # diff
  for (int i = 0; i < n - 1; i++)
    if (z[i] != z[i + 1])
      order[top++] = i;
  order[top++] = n - 1;
  n = top; // Size of y/z -> sizeof tps/fps

  const auto fp = z; // Reuse
  for (int i = 0; i < n; i++)
  {
    tp[i] = tp[order[i]];         // order is mono. inc.
    fp[i] = 1 + order[i] - tp[i]; // Type conversion prevents vectorization
  }
  delete[] order;

  const auto tpn = tp[n - 1], fpn = fp[n - 1];
  for (int i = 0; i < n; i++)
  { // Vectorization
    tp[i] /= tpn;
    fp[i] /= fpn;
  }

  auto area = tp[0] * fp[0] / 2; // The first triangle from origin;
  double partial = 0;            // For Kahan summation
  for (int i = 1; i < n; i++)
  {
    const auto x = (fp[i] - fp[i - 1]) * (tp[i] + tp[i - 1]) / 2 - partial;
    const auto sum = area + x;
    partial = (sum - area) - x;
    area = sum;
  }

  delete[] tp;
  delete[] fp;

  return area;
}

template <typename TP>
std::time_t to_time_t(TP tp)
{
  auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() +
                                                      system_clock::now());
  return system_clock::to_time_t(sctp);
}

template <typename InputIt,
          typename T = typename std::iterator_traits<InputIt>::value_type>
std::vector<std::vector<T>> partitionChunk(InputIt first, InputIt last,
                                           unsigned size)
{
  std::vector<std::vector<T>> result;
  std::vector<T> *batch{};
  for (unsigned index = 0, row = 0; first != last; ++first, ++index)
  {
    if ((index % size) == 0)
    {
      result.resize(++row);
      batch = &result.back();
      batch->reserve(size);
    }
    batch->push_back(*first);
  }
  return result;
}

std::time_t getTimeStampNow();

template <typename T>
std::vector<T> flatten(const std::vector<std::vector<T>> &vec2D)
{
  std::vector<T> result;
  for (const auto &row : vec2D)
  {
    result.insert(result.end(), row.begin(), row.end());
  }
  return result;
}

template <typename T>
void print2DVector(const std::vector<std::vector<T>> &vec)
{
  for (const auto &row : vec)
  {
    for (const T &elem : row)
    {
      std::cout << elem << " ";
    }
    std::cout << "\n";
  }
}

void calculate_embedding();

bool are_equal_double(double a, double b, double epsilon = 1e-5);

template <typename T>
std::vector<T> get_subvector(const std::vector<T> &input, int n)
{
  // Get the first n elements of the vector, if n is greater than the size of the vector, return the entire vector
  if (input.size() <= n)
  {
    return input;
  }
  else
  {
    // Return the first n elements
    return std::vector<T>(input.begin(), input.begin() + n);
  }
}

// double eigen_cosine_similarity(const VectorXd &A, const VectorXd &B);                                // this score between -1 and 1, the higher the score, the more similar the two vectors are
// double normalized_similarity_score_based_on_cosine_similarity(const VectorXd &A, const VectorXd &B); // this score between 0 and 1, the higher the score, the more similar the two vectors are
// VectorXd vectorToEigentVectorXd(const std::vector<double> &vec);

double normalized_similarity_score_based_on_cosine_similarity(const std::vector<double> &A, const std::vector<double> &B);

float randomFloatBetweenZeroAndOne();
double stringToDouble(const std::string &str);

std::string generateSHA256Hash(const std::vector<double> &vec, const std::string &impression_id);
std::string arrayToString(const std::vector<int> &arr);
std::vector<int> stringToArray(const std::string &str);

std::vector<int> find_elements_in_b_not_in_a(const std::vector<int> &a, const std::vector<int> &b);