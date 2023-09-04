#include "httpconn.h"
#include "log/log.h"
#include <cstring>
#include <cassert>
#include <errno.h>
bool HttpConn::isET_ = false;

HttpConn::HttpConn(int fd, char *ip, short port) :
    fd_(fd), clientPort_(port), isClose_(false), iovCnt_(0) {
    assert(ip != nullptr);
    strncpy(clientIp_, ip, INET_ADDRSTRLEN);
}

void HttpConn::close() {
    if (!isClose_) {
        ::close(fd_);
        iovCnt_ = 0;
        isClose_ = true;
        readBuffer_.retrieveAll();
        writeBuffer_.retrieveAll();
    }
}

int HttpConn::read(int &saveErrno) {
    assert(!isClose_);
    int readSize = 0;
    do {
        int len =
            ::read(fd_, readBuffer_.beginWrite(), readBuffer_.writableBytes());
        if (len == -1) {
            saveErrno = errno;
            if (errno != EAGAIN) {
                LOG_INFO(
                    "ip: %s port: %d: read error, errno: %d, error info: %s",
                    clientIp_, clientPort_, saveErrno, strerror(saveErrno));
                return -1;
            } else {
                break;
            }
        } else if (len == 0) {
            LOG_INFO("ip: %s port: %d: disconnected", clientIp_, clientPort_);
            return 0;
        } else {
            readSize += len;
            readBuffer_.hasWritten(len);
            readBuffer_.ensureWriteable(1024);
        }
    } while (isET_);
    return readSize;
}

int HttpConn::write(int &saveErrno) {
    assert(!isClose_);
    
    return 0;
}

bool HttpConn::process() {
    return true;
}

bool HttpConn::isKeepAlive() {
    return false;
}

bool HttpConn::needWrite() {
    return false;
}