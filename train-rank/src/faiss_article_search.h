#ifndef FAISS_ARTICLE_SEARCH_H
#define FAISS_ARTICLE_SEARCH_H

#include <faiss/IndexFlat.h>
#include <vector>
#include <cmath>
#include "entity/impression.h"

class FAISSArticleSearch
{
private:
    faiss::IndexFlatL2 *index;                      // FAISS index, based on Euclidean distance
    std::vector<std::vector<float>> articleVectors; // Article vector library
    int d;                                          // Dimension of the vectors
    int nb;                                         // Number of vectors in the vector library

    // Vector normalization function
    void normalizeVectors(std::vector<float> &vec);

public:
    // Constructor, accepts a 2D vector (article vector library)
    FAISSArticleSearch(std::vector<std::vector<float>> &vectors);
    std::pair<int, float> findMostSimilarArticle(const std::vector<double> &queryVec);
    FAISSArticleSearch(const std::vector<Impression> &impressions);

    // Query method, returns the row number and cosine distance of the most similar article to the given article vector
    std::pair<int, float> findMostSimilarArticle(const std::vector<float> &queryVec);

    // Destructor
    ~FAISSArticleSearch();
};

#endif // FAISS_ARTICLE_SEARCH_H
