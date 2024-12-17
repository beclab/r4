#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>
using namespace std::chrono;

// DECLARE_string(model_path_root);

#include "easylogging++.h"
// using namespace std;
static const char TERMINUS_RECOMMEND_SOURCE_NAME[] =
    "TERMINUS_RECOMMEND_SOURCE_NAME";
static const char KNOWLEDGE_BASE_API_URL[] = "KNOWLEDGE_BASE_API_URL";
static const char TERMINUS_RECOMMEND_EMBEDDING_NAME[] =
    "TERMINUS_RECOMMEND_EMBEDDING_NAME";
static char RECOMMEND_MODEL_ROOT[] = "/opt/rank_model";
static char TERMINUS_RECOMMEND_EMBEDDING_DIMENSION[] =
    "TERMINUS_RECOMMEND_EMBEDDING_DIMENSION";

void init_log();

void printVector(const std::vector<std::string> &a);

bool isStringEmptyOrWhitespace(const std::string &str);

int countStringToken(const std::string &content);

bool isConvertibleToInt(const std::string &str);

std::string envOrBlank(const char *env);

int getEnvInt(const char *envVar, int defaultValue);

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