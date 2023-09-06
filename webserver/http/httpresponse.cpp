#include "httpresponse.h"
#include <bits/types/FILE.h>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <unordered_map>

static const std::unordered_map<int, std::string> CODE2MSG = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const static std::unordered_map<std::string, std::string> SUFFIX2TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const static std::unordered_map<int, std::string> CODE2PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
    {500, "/500.html"},
};

HttpResponse::HttpResponse(const std::string &staticPath) :
    code_(-1), isKeepAlive_(false), fd_(-1), offset_(0),
    staticPath_(staticPath) {
}

HttpResponse::~HttpResponse() {
    close();
}

void HttpResponse::close() {
    if (fd_ > 0) {
        ::close(fd_);
        fd_ = -1;
    }
    fileInfo_ = {0};
    offset_ = 0;
}

void HttpResponse::reset(int code, bool isKeepAlive, const std::string &path) {
    close();
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    fileInfo_ = {0};
}

void HttpResponse::makeResponse(Buffer &buffer) {
    assert(code_ > 0 && !path_.empty());
    preProcess();
    makeResponseLine(buffer);
    makeResponseHeader(buffer);
    makeResponseContent(buffer);
}

void HttpResponse::makeResponseLine(Buffer &buffer) {
    std::string status;
    if (CODE2MSG.find(code_) != CODE2MSG.end()) {
        status += "HTTP/1.1 " + std::to_string(code_) + " " + CODE2MSG.at(code_)
                  + "\r\n";
    } else {
        status += "HTTP/1.1 400 Bad Request\r\n";
    }
    buffer.append(status);
}

void HttpResponse::makeResponseHeader(Buffer &buffer) {
    buffer.append("Connection: ");
    if (isKeepAlive_) {
        buffer.append("keep-alive\r\n");
        buffer.append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buffer.append("close\r\n");
    }
    std::string type = "Content-type: ";
    auto pointPos = path_.find_last_of('.');
    if (pointPos == std::string::npos) {
        type += "text/plain";
    } else {
        auto iter = SUFFIX2TYPE.find(path_.substr(pointPos));
        if (iter == SUFFIX2TYPE.end()) {
            type += "text/plain";
        } else {
            type += iter->second;
        }
    }
    type.append("\r\n");
    buffer.append(type);
}

void HttpResponse::preProcess() {
    if (path_ == "/") { path_ = "/index.html"; }

    if (0 != stat((staticPath_ + path_).c_str(), &fileInfo_)) {
        code_ = 404;
    } else {
        if (access((staticPath_ + path_).c_str(), R_OK) != 0) { code_ = 403; }
    }
    if (CODE2PATH.count(code_) == 1) { path_ = CODE2PATH.at(code_); }
}

void HttpResponse::makeResponseContent(Buffer &buffer) {
    fd_ = open((staticPath_ + path_).c_str(), O_RDONLY);
    if (fd_ < 0) {
        std::string body = make404Response();
        buffer.append("Content-length: " + std::to_string(body.size())
                      + "\r\n\r\n");
        buffer.append(body);
        return;
    } else {
        stat((staticPath_ + path_).c_str(), &fileInfo_);
    }

    buffer.append("Content-length: " + std::to_string(fileInfo_.st_size)
                  + "\r\n\r\n");
}

std::string HttpResponse::make404Response() {
    std::string body;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    body += std::to_string(code_) + " : " + CODE2MSG.at(code_);
    body += "<hr><em>Web Server</em>\n</body></html>";
    return body;
}