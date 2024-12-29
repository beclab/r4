#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <iostream>

using namespace utility;              // Common utilities like string conversions
using namespace web;                  // Common features like URIs.
using namespace web::http;            // Common HTTP functionality
using namespace web::http::client;    // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

int main()
{
    // Create a file stream to receive the response data
    auto fileStream = std::make_shared<ostream>();

    // Open the file stream
    pplx::task<void> requestTask = fstream::open_ostream(U("results.html")).then([=](ostream outFile)
                                                                                 {
        *fileStream = outFile;

        // Create an HTTP request client
        http_client client(U("http://www.google.com"));

        // Create and send a GET request
        return client.request(methods::GET); })

                                       // Handle the response
                                       .then([=](http_response response) -> pplx::task<utility::string_t>
                                             {
        // Check the response status code
        if (response.status_code() != status_codes::OK) {
            throw std::runtime_error("Request failed");
        }

        // Output the response status code
        std::wcout << L"Response status code: " << response.status_code() << std::endl;

        // Return the response body
        return response.extract_string(); })

                                       // Write the response body to the file
                                       .then([=](utility::string_t body)
                                             {
        // *fileStream << "body";
        return fileStream->close(); });

    // Wait for the task to complete
    try
    {
        requestTask.wait();
    }
    catch (const std::exception &e)
    {
        std::wcout << L"Error: " << e.what() << std::endl;
    }

    return 0;
}
