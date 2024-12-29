

#include "userembedding_calculation.h"
#include <eigen3/Eigen/Dense>
#include <vector>
#include "rssrank.h"
using namespace Eigen;

std::vector<double> calcluateUserLongTermEmbedding(const std::vector<Impression> &impressions)
{
    if (impressions.empty())
    {
        return {};
    }
    int row = impressions.size();
    int col = impressions[0].embedding.value().size();
    MatrixXf embeddings(row, col);

    // Fill these vectors
    for (int i = 0; i < row; ++i)
    {
        for (int j = 0; j < col; ++j)
        {
            embeddings(i, j) = impressions[i].embedding.value()[j];
        }
    }
    // Sum the vectors
    VectorXf eig_vec = embeddings.colwise().sum();
    eig_vec = eig_vec / eig_vec.norm();
    return std::vector<double>(eig_vec.data(), eig_vec.data() + eig_vec.size());
}

std::vector<double> calcluateUserShortTermEmbedding(const std::vector<Impression> &impressions, bool with_weight)
{
    if (impressions.empty())
    {
        return {};
    }
    int row = impressions.size();
    int col = impressions[0].embedding.value().size();
    MatrixXf embeddings(row, col);

    // Fill these vectors
    for (int i = 0; i < row; ++i)
    {
        for (int j = 0; j < col; ++j)
        {
            embeddings(i, j) = impressions[i].embedding.value()[j];
        }
    }
    if (with_weight)
    {
        // Sum the vectors
        for (int i = 0; i < row; ++i)
        {
            embeddings.row(i) = embeddings.row(i) * rssrank::getSpecificImpressionScore(impressions[i]);
        }
    }
    // Sum the vectors
    VectorXf eig_vec = embeddings.colwise().sum();
    eig_vec = eig_vec / eig_vec.norm();
    return std::vector<double>(eig_vec.data(), eig_vec.data() + eig_vec.size());
}
