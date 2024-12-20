#include "faiss_article_search.h"
#include <vector>
#include <iostream>
using namespace std;

void FAISSArticleSearch::normalizeVectors(std::vector<float> &vec)
{
    float norm = 0;
    for (float val : vec)
    {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    if (norm > 0)
    {
        for (float &val : vec)
        {
            val /= norm; // Normalize
        }
    }
}

void printVector(const std::vector<std::vector<float>> &vec)
{
    for (const auto &row : vec)
    {
        for (float value : row)
        {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

FAISSArticleSearch::FAISSArticleSearch(std::vector<std::vector<float>> &vectors)
{
    d = vectors[0].size(); // Get the dimension of the vectors
    nb = vectors.size();   // Get the number of vectors

    for (vector<float> &vec : vectors)
    {
        normalizeVectors(vec); // Normalize
    }
    printVector(vectors);
    // Convert 2D vectors to a format acceptable by FAISS
    std::vector<float> flatVectors(nb * d);
    for (int i = 0; i < nb; ++i)
    {
        for (int j = 0; j < d; ++j)
        {
            flatVectors[i * d + j] = vectors[i][j];
        }
    }

    // faiss::normalize_L2(nb, d, flatVectors.data());

    // Create FAISS index
    index = new faiss::IndexFlatL2(d); // Use Euclidean distance (L2 distance)

    // Add normalized vectors to the FAISS index
    index->add(nb, flatVectors.data()); // Add vectors to the index
}

std::pair<int, float> FAISSArticleSearch::findMostSimilarArticle(const std::vector<float> &queryVec)
{
    // Normalize the query vector
    std::vector<float> normalizedQuery = queryVec;
    normalizeVectors(normalizedQuery);

    // Perform the search, returning the k most similar articles
    int k = 1;                           // Only return the most similar article
    std::vector<float> distances(k);     // Store distances
    std::vector<faiss::idx_t> labels(k); // Store article indices

    // Query the FAISS index
    index->search(1, normalizedQuery.data(), k, distances.data(), labels.data());

    // Return the nearest article index and distance (cosine distance is equivalent to Euclidean distance)
    return {labels[0], distances[0]};
}

FAISSArticleSearch::~FAISSArticleSearch()
{
    delete index;
}
