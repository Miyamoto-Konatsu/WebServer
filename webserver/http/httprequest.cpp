#include "httprequest.h"
#include <algorithm>
#include <regex>
#include <string>
#include <unordered_map>

const static std::unordered_map<std::string, std::string> DEFAULT_HTML{
    {"/index", "/index.html"},     {"/register", "/register.html"},
    {"/picture", "/picture.html"}, {"/video", "/video.html"},
    {"/login", "/login.html"},      {"/welcome", "/welcome.html"}};

void HttpRequest::reset() {
    HttpRequest temp;
    std::swap(*this, temp);
}

bool HttpRequest::isKeepAlive() {
    if (header_.count("Connection") == 1) {
        return header_["Connection"] == "keep-alive" && version_ == "HTTP/1.1";
    }
    return false;
}

const char *HttpRequest::findCRLF(const Buffer &buffer) const {
    const char *kCRLF = "\r\n";
    const char *crlf =
        std::search(buffer.peek(), buffer.beginWrite(), kCRLF, kCRLF + 2);
    return crlf == buffer.beginWrite() ? nullptr : crlf;
}

bool HttpRequest::parseRequest(Buffer &buffer, bool &hasParseError) {
    hasParseError = false;
    if (buffer.readableBytes() <= 0) { return false; }

    while (state_ != HttpRequestParseState::kGotAll) {
        std::string line;

        if (state_ == HttpRequestParseState::kExpectBody) {
            auto readableBytes = buffer.readableBytes();
            auto bodyRestLength =
                stoi(header_["Content-Length"]) - body_.size();
            line = std::string(buffer.peek(),
                               buffer.peek()
                                   + std::min(readableBytes, bodyRestLength));
            buffer.retrieve(line.size());
        } else {
            const char *crlf = findCRLF(buffer);
            if (crlf == nullptr) {
                hasParseError = false;
                return false;
            }
            line = std::string(buffer.peek(), crlf);
            buffer.retrieve(line.size() + 2);
        }

        switch (state_) {
        case HttpRequestParseState::kExpectRequestLine: {
            if (!processRequestLine(line)) {
                hasParseError = true;
                return false;
            }
            if (path_ == "/") { path_ = "/index"; }
            if (DEFAULT_HTML.count(path_) == 1) {
                path_ = DEFAULT_HTML.at(path_);
            }
            state_ = HttpRequestParseState::kExpectHeaders;
            break;
        }
        case HttpRequestParseState::kExpectHeaders: {
            if (line.empty()) {
                if (hasBody()) {
                    state_ = HttpRequestParseState::kExpectBody;
                } else {
                    state_ = HttpRequestParseState::kGotAll;
                }
            } else {
                if (!processHeader(line)) {
                    hasParseError = true;
                    return false;
                }
            }
            break;
        }
        case HttpRequestParseState::kExpectBody: {
            if (!processBody(line)) { return false; }
            state_ = HttpRequestParseState::kGotAll;
            break;
        }
        case HttpRequestParseState::kGotAll: {
            break;
        }
        }
    }
    return true;
}

bool HttpRequest::processBody(const std::string &line) {
    body_ += line;
    if (header_.count("Content-Length") == 1) {
        if (!std::regex_match(header_["Content-Length"], std::regex("\\d+"))) {
            throw std::runtime_error("Content-Length is not a number");
        }
        int contentLength = stoi(header_["Content-Length"]);
        if (body_.size() == contentLength) { return true; }
    }
    return false;
}

bool HttpRequest::processRequestLine(const std::string &requestLine) {
    std::regex pattern(R"((\w+) (\S+) (\S+))");

    // 创建正则表达式匹配结果对象
    std::smatch matches;

    // 使用正则表达式匹配HTTP请求行
    if (std::regex_match(requestLine, matches, pattern)) {
        // 第一个捕获组匹配方法（GET）
        method_ = matches[1];
        // 第二个捕获组匹配路径（/example/path）
        path_ = matches[2];
        // 第三个捕获组匹配HTTP版本（HTTP/1.1）
        version_ = matches[3];
        return true;
    } else {
        return false;
    }
}

bool HttpRequest::processHeader(const std::string &headerLine) {
    std::regex pattern(R"(([\w-]+): (.+))");

    // 创建正则表达式匹配结果对象
    std::smatch matches;

    // 使用正则表达式匹配HTTP请求行
    if (std::regex_match(headerLine, matches, pattern)) {
        // 第一个捕获组匹配方法（GET）
        header_[matches[1]] = matches[2];
        return true;
    } else {
        return false;
    }
}

bool HttpRequest::hasBody() const {
    if (header_.count("Content-Length") == 1) { return true; }
    return false;
}
/*
\\: 反斜杠 \ 用于转义特殊字符，使其成为普通字符。例如，\\. 匹配一个实际的点号
.。

\.: 匹配任何字符，除了换行符 \n。点号 . 表示匹配除了换行符外的任何单个字符。

\d: 匹配一个数字字符，等同于 [0-9]。

\D: 匹配一个非数字字符，等同于 [^0-9]。

\w: 匹配一个单词字符（字母、数字或下划线），等同于 [a-zA-Z0-9_]。

\W: 匹配一个非单词字符，等同于 [^a-zA-Z0-9_]。

\s: 匹配一个空白字符（空格、制表符、换行符等），等同于 [\t\n\f\r\p{Z}]，其中
\p{Z} 匹配 Unicode 中的空白字符。

\S: 匹配一个非空白字符，等同于 [^\t\n\f\r\p{Z}]。

\b: 匹配一个单词边界，通常用于确保单词的完整匹配。

\B: 匹配一个非单词边界。

\n: 匹配换行符。

\r: 匹配回车符。

\t: 匹配制表符。
*/