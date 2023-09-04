#pragma once
#include "log/log.h"
#include "buffer/buffer.h"
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>

class HttpConn {
public:
    HttpConn(int fd, char *ip, short port);
    int getFd() const {
        return fd_;
    }

    void close();

    int read(int &saveErrno);

    int write(int &saveErrno);

    void setET() {
        isET_ = true;
    }

    bool process();

    bool isKeepAlive();

    bool needWrite();

private:
    static bool isET_;

    int fd_;

    char clientIp_[INET_ADDRSTRLEN];
    short clientPort_;

    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuffer_;
    Buffer writeBuffer_;
};