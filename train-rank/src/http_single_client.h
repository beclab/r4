// http_client_singleton.h
#ifndef HTTP_CLIENT_SINGLETON_H
#define HTTP_CLIENT_SINGLETON_H

#include <cpprest/http_client.h>
#include <string>

class HttpClientSingleton
{
public:
    // Disable copy constructor and assignment operator
    HttpClientSingleton(const HttpClientSingleton &) = delete;
    HttpClientSingleton &operator=(const HttpClientSingleton &) = delete;

    // Get the unique instance
    static web::http::client::http_client *get_instance();

private:
    // Private constructor
    HttpClientSingleton();
    ~HttpClientSingleton();

    // Get URL from environment
    std::string get_url_from_env();

    // http_client instance
    web::http::client::http_client *client;
};

#endif // HTTP_CLIENT_SINGLETON_H
