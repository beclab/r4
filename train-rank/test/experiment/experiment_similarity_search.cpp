#include <iostream>

int main()
{
    // The input is a sqlite db path, a folder path storing user summary embeddings, and an experiment output path

    // Read all summary embeddings from the path storing summary embeddings, store them in a map where the key is the summary id and the value is a vector<float> representing the summary embedding

    // Randomly select 50 ids from all ids as query ids, and the rest as gallery ids

    // Load the embeddings of gallery ids into faiss search, construct faiss search with std::vector<std::vector<float>> &vectors, record the row_index corresponding to each id and the id corresponding to each row_index

    // For each query id, calculate the similarity between its embedding and all embeddings in the gallery, return the top 3 gallery ids

    // Store the top 3 gallery ids corresponding to each query id in the form of HashMap<string, vector<string>>

    // Save the final result in the form of an excel file
    std::cout << "hello world" << std::endl;
}