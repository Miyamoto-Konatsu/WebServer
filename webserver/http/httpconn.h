#pragma once
#include "http/httprequest.h"
#include "log/log.h"
#include "buffer/buffer.h"
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>
#include "httpresponse.h"
class HttpConn {
public:
    HttpConn(int fd, char *ip, short port, std::string srcPath);
    ~HttpConn();

    int getFd() const {
        return fd_;
    }

    void close();

    int read(int &saveErrno);

    int write(int &saveErrno);

    static void setET() {
        isET_ = true;
    }

    bool process();

    bool isKeepAlive();

    bool needWrite();

private:
    static bool isET_;

    int fd_;

    char clientIp_[INET_ADDRSTRLEN];
    unsigned short clientPort_;

    bool isClose_;

    std::string srcPath_;

    Buffer readBuffer_;
    Buffer writeBuffer_;

    HttpRequest request_;
    HttpResponse response_;
};