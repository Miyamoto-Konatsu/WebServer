#include "buffer/buffer.h"
#include "http/httprequest.h"
#include <iostream>
#include <queue>
#include <string>

// 测试不带请求体的HTTP请求，一次性读取
void test1() {
    std::string httpRequest = "GET /example/path HTTP/1.1\r\n"
                              "Host: www.example.com\r\n"
                              "Accept: */*\r\n"
                              "Accept-Encoding: gzip, deflate\r\n"
                              "Connection: keep-alive\r\n"
                              "\r\n";

    // 创建HttpRequest对象来解析HTTP请求
    HttpRequest request;
    Buffer buffer;
    buffer.append(httpRequest);
    bool hasParseError = false;
    request.parseRequest(buffer, hasParseError);

    // 打印HTTP请求方法
    std::cout << "Method: " << request.getMethod() << std::endl;

    // 打印HTTP请求路径
    std::cout << "Path: " << request.getPath() << std::endl;

    // 打印HTTP请求版本
    std::cout << "Version: " << request.getVersion() << std::endl;

    // 打印HTTP请求头部
    for (const auto &header : request.headers()) {
        std::cout << header.first << ": " << header.second << std::endl;
    }
}

// 测试带有请求体的HTTP请求，一次性读取
void test2() {
    std::string httpRequest = "POST /example/path HTTP/1.1\r\n"
                              "Host: www.example.com\r\n"
                              "Accept: */*\r\n"
                              "Accept-Encoding: gzip, deflate\r\n"
                              "Connection: keep-alive\r\n"
                              "Content-Length: 10\r\n"
                              "\r\n"
                              "1234567890";
    HttpRequest request;
    Buffer buffer;
    buffer.append(httpRequest);
    bool hasParseError = false;
    request.parseRequest(buffer, hasParseError);

    // 打印HTTP请求方法
    std::cout << "Method: " << request.getMethod() << std::endl;

    // 打印HTTP请求路径
    std::cout << "Path: " << request.getPath() << std::endl;

    // 打印HTTP请求版本
    std::cout << "Version: " << request.getVersion() << std::endl;

    // 打印HTTP请求头部
    for (const auto &header : request.headers()) {
        std::cout << header.first << ": " << header.second << std::endl;
    }

    // 打印HTTP请求体
    std::cout << "Body: " << request.getBody() << std::endl;
}

// 测试带不有请求体的HTTP请求，分多次读取
void test3() {
    std::string httpRequest1 = "GET /example/path HTTP/1.1\r\n"
                               "Host: www.example.com\r\n"
                               "Accept: */*\r\n";
    std::string httpRequest2 = "Accept-Encoding: gzip, deflate\r\n"
                               "Connection: keep-alive\r\n"
                               "\r\n";
    std::queue<std::string> q;
    q.push(httpRequest1);
    q.push(httpRequest2);

    HttpRequest request;
    Buffer buffer;

    bool hasParseError = false;
    while (!request.parseRequest(buffer, hasParseError)) {
        if (hasParseError) {
            std::cout << "parse error" << std::endl;
            return;
        }
        buffer.append(q.front());
        q.pop();
    }

    // 打印HTTP请求方法
    std::cout << "Method: " << request.getMethod() << std::endl;

    // 打印HTTP请求路径
    std::cout << "Path: " << request.getPath() << std::endl;

    // 打印HTTP请求版本
    std::cout << "Version: " << request.getVersion() << std::endl;

    // 打印HTTP请求头部
    for (const auto &header : request.headers()) {
        std::cout << header.first << ": " << header.second << std::endl;
    }
}

// 测试带有请求体的HTTP请求，分多次读取
void test4() {
    std::string httpRequest1 = "POST /example/path HTTP/1.1\r\n"
                               "Host: www.example.com\r\n"
                               "Accept: */*\r\n";
    std::string httpRequest2 = "Connection: keep-alive\r\n"
                               "Content-Length: 10\r\n";
    std::string httpRequest3 = "\r\n";
    std::string httpRequest4 = "1234567";
    std::string httpRequest5 = "890";
    std::queue<std::string> q;
    q.push(httpRequest1);
    q.push(httpRequest2);
    q.push(httpRequest3);
    q.push(httpRequest4);
    q.push(httpRequest5);
    HttpRequest request;
    Buffer buffer;

    bool hasParseError = false;
    while (!request.parseRequest(buffer, hasParseError)) {
        if (hasParseError) {
            std::cout << "parse error" << std::endl;
            return;
        }
        buffer.append(q.front());
        q.pop();
    }

    // 打印HTTP请求方法
    std::cout << "Method: " << request.getMethod() << std::endl;

    // 打印HTTP请求路径
    std::cout << "Path: " << request.getPath() << std::endl;

    // 打印HTTP请求版本
    std::cout << "Version: " << request.getVersion() << std::endl;

    // 打印HTTP请求头部
    for (const auto &header : request.headers()) {
        std::cout << header.first << ": " << header.second << std::endl;
    }

    // 打印HTTP请求体
    std::cout << "Body: " << request.getBody() << std::endl;
}

// 测试带有请求体的HTTP请求，分多次读取，且读取的数据有错误
void test5() {
    std::string httpRequest1 = "POST /example/path HTTP/1.1\r\n"
                               "Host: www.example.com\r\n"
                               "Accept: */*\r\n";
    std::string httpRequest2 = "Connection: keep-alive\r\n"
                               "Content-Length:10\r\n"; // 错误的Content-Length
    std::string httpRequest3 = "\r\n";
    std::string httpRequest4 = "1234567";
    std::string httpRequest5 = "890";
    std::queue<std::string> q;
    q.push(httpRequest1);
    q.push(httpRequest2);
    q.push(httpRequest3);
    q.push(httpRequest4);
    q.push(httpRequest5);
    HttpRequest request;
    Buffer buffer;

    bool hasParseError = false;
    while (!request.parseRequest(buffer, hasParseError)) {
        if (hasParseError) {
            std::cout << "parse error" << std::endl;
            return;
        }
        buffer.append(q.front());
        q.pop();
    }

    // 打印HTTP请求方法
    std::cout << "Method: " << request.getMethod() << std::endl;

    // 打印HTTP请求路径
    std::cout << "Path: " << request.getPath() << std::endl;

    // 打印HTTP请求版本
    std::cout << "Version: " << request.getVersion() << std::endl;

    // 打印HTTP请求头部
    for (const auto &header : request.headers()) {
        std::cout << header.first << ": " << header.second << std::endl;
    }

    // 打印HTTP请求体
    std::cout << "Body: " << request.getBody() << std::endl;
}
int main() {
    std::cout << "test1" << std::endl;
    test1();
    std::cout << std::endl << "test2" << std::endl;
    test2();
    std::cout << std::endl << "test3" << std::endl;
    test3();
    std::cout << std::endl << "test4" << std::endl;
    test4();
    std::cout << std::endl << "test5" << std::endl;
    test5();
    return 0;
}