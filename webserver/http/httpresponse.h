#pragma once
#include <cassert>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include "buffer/buffer.h"

class HttpResponse {
public:
    HttpResponse(const std::string &staticPath);
    ~HttpResponse();

    void reset(int code, bool isKeepAlive, const std::string &path);

    void makeResponse(Buffer &buffer);

    int getFd() const {
        return fd_;
    }

    off_t &getOffset() {
        return offset_;
    }

    size_t fileBytesNeedWrite() {
        if (fd_ <= 0) return 0;
        assert(offset_ <= fileInfo_.st_size);
        return fileInfo_.st_size - offset_;
    }

private:
    void makeResponseLine(Buffer &buffer);
    void makeResponseHeader(Buffer &buffer);
    void makeResponseContent(Buffer &buffer);
    std::string make404Response();
    void preProcess();
    void close();

private:
    int code_;
    bool isKeepAlive_;
    std::string path_;
    const std::string staticPath_;
    int fd_;
    off_t offset_;

    struct stat fileInfo_;
};