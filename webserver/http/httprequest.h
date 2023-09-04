#pragma once
#include "buffer/buffer.h"
#include <unordered_map>
#include <unordered_map>
class HttpRequest {
    enum class HttpRequestParseState {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

public:
    bool isKeepAlive();

    void reset();

    bool parseRequest(Buffer &buffer, bool &hasParseError);

    std::string getPath() const {
        return path_;
    }

    std::string getMethod() const {
        return method_;
    }

    std::string getVersion() const {
        return version_;
    }

    std::string getHeader(const std::string &key) const {
        std::string header;
        auto iter = header_.find(key);
        if (iter != header_.end()) {
            header = iter->second;
        }
        return header;
    }

    const std::unordered_map<std::string, std::string> &headers() const {
        return header_;
    }

    std::string getBody() const {
        return body_;
    }

private:
    bool processRequestLine(const std::string &line);

    bool processHeader(const std::string &line);

    bool processBody(const std::string &line);

    const char *findCRLF(const Buffer &buffer) const;

    bool hasBody() const;

private:
    HttpRequestParseState state_=HttpRequestParseState::kExpectRequestLine;

    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> header_;
    std::string body_;
};