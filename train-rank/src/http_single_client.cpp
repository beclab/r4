// http_client_singleton.cpp
#include "http_single_client.h"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include <cpprest/http_client.h>
using namespace utility;
using namespace web::http;
using namespace web::http::client;

// Private constructor: initialize http_client
HttpClientSingleton::HttpClientSingleton()
{
    // http_client client(U(std::getenv("KNOWLEDGE_BASE_API_URL")));
    // this->client = client;

    this->client = new http_client(U(get_url_from_env()));
}

// Destructor: release http_client
HttpClientSingleton::~HttpClientSingleton()
{
    delete this->client;
}

// Method to get URL
std::string HttpClientSingleton::get_url_from_env()
{
    const char *url = std::getenv("KNOWLEDGE_BASE_API_URL");
    if (!url)
    {
        throw std::runtime_error("Environment variable KNOWLEDGE_BASE_API_URL is not set.");
    }
    return std::string(url);
}

// Get the unique http_client instance
web::http::client::http_client *HttpClientSingleton::get_instance()
{
    static HttpClientSingleton instance; // Static local variable ensures singleton
    return instance.client;
}
