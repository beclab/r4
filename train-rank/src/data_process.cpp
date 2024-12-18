#include "data_process.h"

#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "easylogging++.h"

bool RandomOverResampleSpecificNumber(int specifical_size,
                                      std::vector<std::string> *lessv)
{
  int original_less_size = lessv->size();
  // int original_more_size = morev.size();
  if (specifical_size == original_less_size)
  {
    LOG(INFO) << " original more vector size equal to less vector size "
              << specifical_size << std::endl;
    return true;
  }
  if (specifical_size < original_less_size)
  {
    LOG(ERROR) << " specifical_size " << specifical_size
               << " small than less vector size " << original_less_size
               << std::endl;
    return false;
  }
  LOG(INFO) << " specifical_size " << specifical_size << "  less vector size "
            << original_less_size << std::endl;
  int gap = specifical_size - lessv->size();
  int quotient = gap / lessv->size();
  int remainder = gap % lessv->size();
  LOG(INFO) << "quotient " << quotient << "remainer " << remainder << std::endl;
  std::vector<std::string> temp;
  if (remainder > 0)
  {
    std::sample(lessv->begin(), lessv->end(), std::back_inserter(temp),
                remainder, std::mt19937{std::random_device{}()});
  }

  if (quotient > 0)
  {
    for (int index = 0; index < original_less_size; index++)
    {
      for (int multiple = 0; multiple < quotient; multiple++)
      {
        lessv->push_back(lessv->at(index));
      }
    }
  }

  lessv->insert(lessv->end(), temp.begin(), temp.end());
  return true;
}

bool RandomOverResample(std::vector<std::string> *morev,
                        std::vector<std::string> *lessv)
{
  int original_less_size = lessv->size();
  int original_more_size = morev->size();
  if (original_more_size == original_less_size)
  {
    LOG(INFO) << " original more vector size equal to less vector size "
              << original_more_size << std::endl;
    return true;
  }
  if (original_more_size < original_less_size)
  {
    LOG(ERROR) << " original more vector size " << original_more_size
               << " small than less vector size " << original_less_size
               << std::endl;
    return false;
  }
  LOG(INFO) << " original more vector size " << original_more_size
            << "  less vector size " << original_less_size << std::endl;
  int gap = morev->size() - lessv->size();
  int quotient = gap / lessv->size();
  int remainder = gap % lessv->size();
  LOG(INFO) << "quotient " << quotient << "remainer " << remainder << std::endl;
  std::vector<std::string> temp;
  if (remainder > 0)
  {
    std::sample(lessv->begin(), lessv->end(), std::back_inserter(temp),
                remainder, std::mt19937{std::random_device{}()});
  }

  if (quotient > 0)
  {
    for (int index = 0; index < original_less_size; index++)
    {
      for (int multiple = 0; multiple < quotient; multiple++)
      {
        lessv->push_back(lessv->at(index));
      }
    }
  }

  lessv->insert(lessv->end(), temp.begin(), temp.end());
  return true;
}

bool ConfigureTrainTestRatioWithoutOverSample(
    const std::unordered_set<std::string> &positive_sample,
    const std::unordered_set<std::string> &negative_sample,
    std::unordered_set<std::string> *train_positive_sample,
    std::unordered_set<std::string> *train_negative_sample,
    std::unordered_set<std::string> *test_positive_sample,
    std::unordered_set<std::string> *test_negative_sample, float ratio)
{
  if (ratio <= 0 || ratio >= 1)
  {
    LOG(ERROR) << "ratio is not valid ratio " << ratio << std::endl;
    return false;
  }
  train_positive_sample->clear();
  train_negative_sample->clear();
  test_positive_sample->clear();
  test_negative_sample->clear();

  LOG(INFO) << " original positive size " << positive_sample.size()
            << "  original negative size " << negative_sample.size()
            << std::endl;
  if (positive_sample.size() < 10)
  {
    LOG(ERROR) << "positive sample is too samll " << positive_sample.size()
               << std::endl;
    return false;
  }
  if (negative_sample.size() < 10)
  {
    LOG(ERROR) << "negative sample is too samll " << negative_sample.size()
               << std::endl;
    return false;
  }
  int train_positive_count = static_cast<int>(ratio * positive_sample.size());
  int train_negative_count = static_cast<int>(ratio * negative_sample.size());
  LOG(INFO) << " train_positive_count " << train_positive_count
            << " train_negative_count" << train_negative_count << std::endl;
  // std::unordered_set<string> train_positive_set;
  // std::unordered_set<string> train_negative_set;
  {
    std::vector<std::string> train_positive;
    std::vector<std::string> train_negative;
    std::sample(positive_sample.begin(), positive_sample.end(),
                std::back_inserter(train_positive), train_positive_count,
                std::mt19937{std::random_device{}()});
    std::sample(negative_sample.begin(), negative_sample.end(),
                std::back_inserter(train_negative), train_negative_count,
                std::mt19937{std::random_device{}()});
    train_positive_sample->insert(train_positive.begin(), train_positive.end());
    train_negative_sample->insert(train_negative.begin(), train_negative.end());

    LOG(INFO) << "train positive size " << train_positive_sample->size()
              << "train negative size " << train_negative_sample->size()
              << std::endl;
  }

  {
    for (const auto &item : positive_sample)
    {
      if (train_positive_sample->count(item) == 0)
      {
        test_positive_sample->insert(item);
      }
    }
    for (const auto &item : negative_sample)
    {
      if (train_negative_sample->count(item) == 0)
      {
        test_negative_sample->insert(item);
      }
    }

    LOG(INFO) << "test_positive_sample  " << test_positive_sample->size()
              << "test_negative_sample " << test_negative_sample->size()
              << std::endl;
  }
  return true;
}

bool ConfigureTrainTestRation(
    const std::unordered_set<std::string> &positive_sample,
    const std::unordered_set<std::string> &negative_sample,
    std::vector<std::string> *train_sample,
    std::vector<std::string> *test_sample, float ratio)
{
  if (ratio <= 0 || ratio >= 1)
  {
    LOG(ERROR) << "ratio is not valid ratio " << ratio << std::endl;
    return false;
  }
  train_sample->clear();
  test_sample->clear();
  LOG(INFO) << " original positive size " << positive_sample.size()
            << "  original negative size " << negative_sample.size()
            << std::endl;
  if (positive_sample.size() < 10)
  {
    LOG(ERROR) << "positive sample is too samll " << positive_sample.size()
               << positive_sample.size() << std::endl;
    return false;
  }
  if (negative_sample.size() < 10)
  {
    LOG(ERROR) << "negative sample is too samll " << negative_sample.size()
               << positive_sample.size() << std::endl;
    return false;
  }
  int train_positive_count = static_cast<int>(ratio * positive_sample.size());
  int train_negative_count = static_cast<int>(ratio * negative_sample.size());
  LOG(INFO) << " train_positive_count " << train_positive_count
            << " train_negative_count" << train_positive_count << std::endl;
  std::unordered_set<std::string> train_positive_set;
  std::unordered_set<std::string> train_negative_set;
  {
    std::vector<std::string> train_positive;
    std::vector<std::string> train_negative;
    std::sample(positive_sample.begin(), positive_sample.end(),
                std::back_inserter(train_positive), train_positive_count,
                std::mt19937{std::random_device{}()});
    std::sample(negative_sample.begin(), negative_sample.end(),
                std::back_inserter(train_negative), train_negative_count,
                std::mt19937{std::random_device{}()});
    train_positive_set.insert(train_positive.begin(), train_positive.end());
    train_negative_set.insert(train_negative.begin(), train_negative.end());
    if (train_positive.size() > train_negative.size())
    {
      RandomOverResample(&train_positive, &train_negative);
    }
    else
    {
      RandomOverResample(&train_negative, &train_positive);
    }
    train_sample->insert(train_sample->end(), train_positive.begin(),
                         train_positive.end());
    train_sample->insert(train_sample->end(), train_negative.begin(),
                         train_negative.end());
    LOG(INFO) << " train_sample size " << train_sample->size() << std::endl;
  }

  {
    std::vector<std::string> test_positive;
    std::vector<std::string> test_negative;
    for (auto item : positive_sample)
    {
      if (train_positive_set.count(item) == 0)
      {
        test_positive.push_back(item);
      }
    }
    for (auto item : negative_sample)
    {
      if (train_negative_set.count(item) == 0)
      {
        test_negative.push_back(item);
      }
    }
    if (test_positive.size() > test_negative.size())
    {
      RandomOverResample(&test_positive, &test_negative);
    }
    else
    {
      RandomOverResample(&test_negative, &test_positive);
    }
    test_sample->insert(test_sample->end(), test_positive.begin(),
                        test_positive.end());
    test_sample->insert(test_sample->end(), test_negative.begin(),
                        test_negative.end());
    LOG(INFO) << " test_sample size " << test_sample->size() << std::endl;
  }
  return true;
}
