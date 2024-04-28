#pragma once
#include <string>
#include <unordered_set>
#include <vector>

bool RandomOverResample(std::vector<std::string>* morev,
                        std::vector<std::string>* lessv);

bool RandomOverResampleSpecificNumber(int specifical_size,
                                      std::vector<std::string>* lessv);

/**
 * @brief
 *
 * @param positive_sample
 * @param negative_sample
 * @param train_sample
 * @param test_sample
 * @param ratio  train sample ratio
 * @return true
 * @return false
 */
bool ConfigureTrainTestRation(
    const std::unordered_set<std::string>& positive_sample,
    const std::unordered_set<std::string>& negative_sample,
    std::vector<std::string>* train_sample,
    std::vector<std::string>* test_sample, float ratio);

bool ConfigureTrainTestRatioWithoutOverSample(
    const std::unordered_set<std::string>& positive_sample,
    const std::unordered_set<std::string>& negative_sample,
    std::unordered_set<std::string>* train_positive_sample,
    std::unordered_set<std::string>* train_negative_sample,
    std::unordered_set<std::string>* test_positive_sample,
    std::unordered_set<std::string>* test_negative_sample, float ratio);
