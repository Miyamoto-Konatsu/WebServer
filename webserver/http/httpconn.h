#pragma once
#include "http/httprequest.h"
#include "log/log.h"
#include "buffer/buffer.h"
#include <bits/types/struct_iovec.h>
#include <memory>
#include <netinet/in.h>
#include "httpresponse.h"
#include <mutex>
#include "server/epoller.h"

class HttpConn {
public:
    HttpConn(int fd, char *ip, unsigned short port, std::string srcPath,
             std::shared_ptr<Epoller> epoller);
    ~HttpConn();

    int getFd();

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
    void throwIfClosed();

private:
    static bool isET_;

    int fd_;
    std::shared_ptr<Epoller> epoller_;

    char clientIp_[INET_ADDRSTRLEN];
    unsigned short clientPort_;

    bool isClose_;
    bool isKeepAlive_;

    std::mutex mtx_;

    std::string srcPath_;

    Buffer readBuffer_;
    Buffer writeBuffer_;

    HttpRequest request_;
    HttpResponse response_;
};