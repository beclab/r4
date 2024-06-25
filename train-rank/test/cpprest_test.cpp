#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <iostream>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

int main() {
    // 创建文件流以接收响应数据
    auto fileStream = std::make_shared<ostream>();

    // 打开文件流
    pplx::task<void> requestTask = fstream::open_ostream(U("results.html")).then([=](ostream outFile) {
        *fileStream = outFile;

        // 创建 HTTP 请求客户端
        http_client client(U("http://www.google.com"));

        // 创建并发送 GET 请求
        return client.request(methods::GET);
    })

    // 处理响应
    .then([=](http_response response) -> pplx::task<utility::string_t> {
        // 检查响应状态码
        if (response.status_code() != status_codes::OK) {
            throw std::runtime_error("请求失败");
        }

        // 输出响应状态码
        std::wcout << L"响应状态码: " << response.status_code() << std::endl;

        // 返回响应主体
        return response.extract_string();
    })

    // 将响应主体写入文件
    .then([=](utility::string_t body) {
        // *fileStream << "body";
        return fileStream->close();
    });

    // 等待任务完成
    try {
        requestTask.wait();
    } catch (const std::exception &e) {
        std::wcout << L"错误: " << e.what() << std::endl;
    }

    return 0;
}
