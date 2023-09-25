#include "httpconn.h"
#include "buffer/buffer.h"
#include "http/httpresponse.h"
#include "log/log.h"
#include <cstring>
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/sendfile.h>

bool HttpConn::isET_ = false;

HttpConn::HttpConn(int fd, char *ip, unsigned short port, std::string srcPath,
                   std::shared_ptr<Epoller> epoller) :
    fd_(fd),
    clientPort_(port), isClose_(false), srcPath_(srcPath),
    epoller_(std::move(epoller)), isKeepAlive_(false), response_(srcPath) {
    assert(ip != nullptr);
    strncpy(clientIp_, ip, INET_ADDRSTRLEN);
    LOG_INFO("new http conn, remote addr = %s ,remote = fd: %hu", clientIp_,
             clientPort_);
}

HttpConn::~HttpConn() {
    close();
}

bool HttpConn::close() {
    LOG_INFO("close http conn, remote addr = %s ,remote = fd: %hu", clientIp_,
             clientPort_);
    auto isLocked = mtx_.try_lock();
    if (!isLocked) { return true; }
    int res = fd_;
    if (!isClose_) {
        ::close(fd_);
        epoller_->delFd(fd_);
        isClose_ = true;
        readBuffer_.retrieveAll();
        writeBuffer_.retrieveAll();
        request_.reset();
        response_.close();
        isKeepAlive_ = false;
    }
    mtx_.unlock();
    return false;
}

int HttpConn::read(int &saveErrno) {
    std::lock_guard<std::mutex> lock(mtx_);
    throwIfClosed();

    assert(!isClose_);
    int readSize = 0;
    do {
        int len =
            ::read(fd_, readBuffer_.beginWrite(), readBuffer_.writableBytes());
        if (len == -1) {
            saveErrno = errno;
            if (errno != EAGAIN) {
                LOG_DEBUG(
                    "ip: %s port: %d: read error, errno: %hu, error info: %s",
                    clientIp_, clientPort_, saveErrno, strerror(saveErrno));
                return -1;
            } else {
                break;
            }
        } else if (len == 0) {
            LOG_DEBUG("ip: %s port: %hu: disconnected", clientIp_, clientPort_);
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
    std::lock_guard<std::mutex> lock(mtx_);
    throwIfClosed();

    assert(!isClose_);
    int writeSize = 0;
    do {
        if (writeBuffer_.readableBytes()) {
            int len =
                ::write(fd_, writeBuffer_.peek(), writeBuffer_.readableBytes());
            if (len == -1) {
                saveErrno = errno;
                if (errno != EAGAIN) {
                    LOG_DEBUG("write error, errno: %d, error info: %s",
                              saveErrno, strerror(saveErrno));
                    return -1;
                } else {
                    break;
                }
            } else {
                writeSize += len;
                writeBuffer_.retrieve(len);
                if (writeBuffer_.readableBytes() == 0) {
                    writeBuffer_.retrieveAll();
                }
            }
        } else if (response_.fileBytesNeedWrite()) {
            ssize_t needWrite = response_.fileBytesNeedWrite();
            // SO_NOSIGPIPE ;
            ssize_t ret = sendfile(fd_, response_.getFd(),
                                   &response_.getOffset(), needWrite);
            LOG_DEBUG("write file bytes, ret: %d", ret);
            if (ret == -1) {
                saveErrno = errno;
                if (errno != EAGAIN) {
                    LOG_DEBUG("write error, errno: %d, error info: %s",
                              saveErrno, strerror(saveErrno));
                    return -1;
                } else {
                    break;
                }
            } else {
                writeSize += ret;
            }
        } else {
            break;
        }
    } while (isET_);

    return writeSize;
}

int HttpConn::getFd() {
    std::lock_guard<std::mutex> lock(mtx_);
    throwIfClosed();

    return fd_;
}
// return true if read all or has error
// return false if not read all
bool HttpConn::process() {
    std::lock_guard<std::mutex> lock(mtx_);
    throwIfClosed();

    bool hasError = false;
    if (request_.parseRequest(readBuffer_, hasError)) {
        isKeepAlive_ = request_.isKeepAlive();
        response_.reset(200, request_.isKeepAlive(), request_.getPath());
        response_.makeResponse(writeBuffer_);
        request_.reset();
        return true;
    } else {
        if (hasError) {
            response_.reset(400, request_.isKeepAlive(), request_.getPath());
            isKeepAlive_ = request_.isKeepAlive();
            response_.makeResponse(writeBuffer_);
            request_.reset();
            return true;
        }
        return false;
    }
}

bool HttpConn::isKeepAlive() {
    std::lock_guard<std::mutex> lock(mtx_);
    throwIfClosed();
    return isKeepAlive_;
}

bool HttpConn::needWrite() {
    std::lock_guard<std::mutex> lock(mtx_);
    throwIfClosed();
    return writeBuffer_.readableBytes() > 0
           || response_.fileBytesNeedWrite() > 0;
}

void HttpConn::throwIfClosed() {
    if (isClose_) { throw std::logic_error("HttpConn has closed"); }
}