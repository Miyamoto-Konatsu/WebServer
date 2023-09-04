#include <iostream>
#include <regex>
#include <string>

int main() {
    std::string httpRequest = "GET /example/path";

    // 创建正则表达式对象来匹配HTTP请求行
    std::regex pattern(R"((\w+) (\S+) (\S+))");

    // 创建正则表达式匹配结果对象
    std::smatch matches;

    // 使用正则表达式匹配HTTP请求行
    if (std::regex_match(httpRequest, matches, pattern)) {
        // 第一个捕获组匹配方法（GET）
        std::string method = matches[1];
        std::cout << "Method: " << method << std::endl;

        // 第二个捕获组匹配路径（/example/path）
        std::string path = matches[2];
        std::cout << "Path: " << path << std::endl;

        // 第三个捕获组匹配HTTP版本（HTTP/1.1）
        std::string version = matches[3];
        std::cout << "Version: " << version << std::endl;
    }

    return 0;
}
