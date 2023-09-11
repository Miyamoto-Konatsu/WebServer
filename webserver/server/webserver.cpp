#include "webserver.h"
#include "http/httpconn.h"
#include "http/httprequest.h"
#include "log/log.h"
#include "pool/threadpool.h"
#include "server/epoller.h"
#include "timer/heaptimer.h"
#include <mutex>
#include <netinet/tcp.h>
#include "sys/socket.h"
#include "unistd.h"
#include <cassert>
#include <cstdio>
#include <errno.h>
#include <stdexcept>
#include <string.h>
#include <memory>
#include <thread>
#include <unistd.h>
#include <sys/socket.h> // Added header for SO_NOSIGPIPE
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
volatile int WebServer::isClosed_ = 0;

static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

WebServer::WebServer(const char *ip, int port, int triggerMode, int timeout,
                     std::string srcPath) :
    ip_(ip),
    port_(port), triggerMode_(triggerMode), httpTimeOut_(timeout),
    srcPath_(srcPath), threadPool_(new ThreadPool(6)), timer_(new HeapTimer),
    epoller_(new Epoller) {
    httpConnTimerMap_.resize(65535);
    httpConnMap_.resize(65535);
    init();
}

// ? 这个关闭资源的顺序是否正确
WebServer::~WebServer() {
    isClosed_ = 1;
    close(listenFd_);
    // 会等待所有任务执行完毕后再关闭线程池
    threadPool_.reset();
    timer_.reset();
    epoller_.reset();
}

void WebServer::init() {
    threadPool_->start();

    initTriggerMode();

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    setNonBlocking(listenFd_);
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip_);
    sa.sin_port = htons(port_);
    if (bind(listenFd_, (const sockaddr *)&sa, sizeof(struct sockaddr))) {
        LOG_ERROR("failed to init listenFd");
        exit(-1);
    }
    if (!epoller_->addFd(listenFd_, listenEvent_)) {
        LOG_ERROR("failed to add listenFd to epoller");
        exit(-1);
    }
    if (listen(listenFd_, 1024)) {
        LOG_ERROR("failed to listen");
        exit(-1);
    }
}

void WebServer::initTriggerMode() {
    httpConnEvent_ = EPOLLERR | EPOLLRDHUP | EPOLLONESHOT;
    listenEvent_ = EPOLLERR | EPOLLIN;
    switch (triggerMode_) {
    case 0: {
        LOG_DEBUG("triggerMode: listenfd is lt, httpconnfd is lt");
        break;
    }
    case 1: {
        listenEvent_ |= EPOLLET;
        LOG_DEBUG("triggerMode: listenfd is et, httpconnfd is lt");
        break;
    }
    case 2: {
        httpConnEvent_ |= EPOLLET;
        LOG_DEBUG("triggerMode: listenfd is lt, httpconnfd is et");
        break;
    }
    case 3: {
        listenEvent_ |= EPOLLET;
        httpConnEvent_ |= EPOLLET;
        LOG_DEBUG("triggerMode: listenfd is et, httpconnfd is et");
        break;
    }
    default: {
        listenEvent_ |= EPOLLET;
        httpConnEvent_ |= EPOLLET;
        LOG_DEBUG("triggerMode: listenfd is et, httpconnfd is et");
        break;
    }
    }
    if (triggerMode_ >= 2) { HttpConn::setET(); }
}

void WebServer::start() {
    int epollTimeOut = -1;
    while (!isClosed_) {
        if (httpTimeOut_ > 0) { epollTimeOut = timer_->getNextTick(); }
        int eventNum = epoller_->epollWait(epollTimeOut);
        if (eventNum < 0) {
            if (errno != EINTR) {
                LOG_ERROR("epoll_wait error");
                break;
            } else {
                continue;
            }
        }
        for (int i = 0; i < eventNum; ++i) {
            int fd = epoller_->getEvent(i).data.fd;
            if (fd == listenFd_) {
                dealListen();
            } else if (epoller_->getEvent(i).events & (EPOLLRDHUP | EPOLLERR)) {
                // todo:现在遇到EPOLLRDHUP和EPOLLERR都是直接关闭连接，后续应该单独处理EPOLLRDHUP事件
                closeHttpConn(httpConnMap_[fd]);
            } else if (epoller_->getEvent(i).events & EPOLLIN) {
                assert(httpConnMap_[fd]);
                dealRead(httpConnMap_[fd]);
            } else if (epoller_->getEvent(i).events & EPOLLOUT) {
                assert(httpConnMap_[fd]);
                dealWrite(httpConnMap_[fd]);
            }
        }
    }
}

// 并发调用
void WebServer::closeHttpConn(std::weak_ptr<HttpConn> httpClientWeak) {
    // if (httpTimeOut_ > 0) {
    //     timer_->delTimerNode(httpConnTimerMap_[httpClientFd]);
    // }
    //
    if (auto httpClient = httpClientWeak.lock()) {
        httpClient->close();
        httpClient.reset();
    }
    // 关闭fd后，epoller会自动删除fd（当该fd的引用计数为0时）
    // int fd = httpClient->close();
    // if (fd > 0) { epoller_->delFd(fd); }
}

void WebServer::dealListen() {
    do {
        struct sockaddr_in addr;
        socklen_t addr_len;
        int httpClientFd =
            accept(listenFd_, (struct sockaddr *)&addr, &addr_len);
        int error_code = errno;
        // printf("Error: %s\n", strerror(error_code));
        if (httpClientFd < 0) break;
        if (httpClientFd >= 65535) {
            close(httpClientFd);
            continue;
        }
        char clientIp[INET_ADDRSTRLEN] = {0};
        unsigned short clientPort;

        clientPort = ntohs(addr.sin_port);

        // 使用inet_ntop函数将IP地址从网络字节顺序转换为字符串表示形式
        if (inet_ntop(AF_INET, &(addr.sin_addr), clientIp, INET_ADDRSTRLEN)
            == NULL) {
            LOG_WARN("inet_ntop error")
            close(httpClientFd);
            continue;
        } else {
            LOG_INFO("accept a connection from %s:%hu\n", clientIp, clientPort);
        }

        httpConnMap_[httpClientFd] = std::make_shared<HttpConn>(
            httpClientFd, clientIp, clientPort, srcPath_, epoller_);

        if (httpTimeOut_ > 0) {
            TimerId timerId = timer_->add(
                std::bind(&WebServer::closeHttpConn, this,
                          httpConnMap_[httpClientFd]),
                this->httpTimeOut_ / 1000, this->httpTimeOut_ % 1000);
            httpConnTimerMap_[httpClientFd] = timerId;
        }

        setNonBlocking(httpClientFd);

        assert(epoller_->addFd(httpClientFd, httpConnEvent_ | EPOLLIN));
    } while (listenEvent_ & EPOLLET);
}

void WebServer::dealRead(std::shared_ptr<HttpConn> &httpConn) {
    assert(httpConn);
    threadPool_->addTask([this, &httpConn]() { onRead(httpConn); });
    resetTimeout(httpConn);
}

void WebServer::dealWrite(std::shared_ptr<HttpConn> &httpConn) {
    assert(httpConn);
    threadPool_->addTask([this, &httpConn]() { onWrite(httpConn); });
    resetTimeout(httpConn);
}

void WebServer::resetTimeout(std::shared_ptr<HttpConn> &httpConn) {
    assert(httpConn);
    if (httpTimeOut_ > 0) {
        try {
            TimerId timerId = httpConnTimerMap_[httpConn->getFd()];
            timer_->adjust(timerId, httpTimeOut_ / 1000, httpTimeOut_ % 1000);
        } catch (const std::logic_error &e) {
            LOG_WARN("%s %s use a closed http conn, exception: %s", __FILE__,
                     __FUNCTION__, e.what());
        }
    }
}

void WebServer::onRead(std::shared_ptr<HttpConn> httpConn) {
    try {
        int readErrno = 0;
        int readSize = httpConn->read(readErrno);
        if (readSize < 0 && readErrno != EAGAIN || readSize == 0) {
            closeHttpConn(httpConn);
            return;
        }
        LOG_DEBUG("read from httpconn, fd: %d, readSize: %d", httpConn->getFd(),
                  readSize);
        process(httpConn);
    } catch (const std::logic_error &e) {
        LOG_WARN("exception: %s", e.what());
        closeHttpConn(httpConn);
    }
}

void WebServer::onWrite(std::shared_ptr<HttpConn> httpConn) {
    assert(httpConn);
    try {
        int writeErrno = 0;
        int writeSize = httpConn->write(writeErrno);
        if (writeSize < 0 && writeErrno != EAGAIN) {
            closeHttpConn(httpConn);
            return;
        }
        LOG_DEBUG("write to httpconn, fd: %d, writeSize: %d", httpConn->getFd(),
                  writeSize);
        if (httpConn->needWrite()) {
            epoller_->modFd(httpConn->getFd(), httpConnEvent_ | EPOLLOUT);
        } else {
            if (httpConn->isKeepAlive()) {
                process(httpConn);
                return;
            }
            closeHttpConn(httpConn);
        }
    } catch (const std::logic_error &e) {
        LOG_WARN("exception: %s", e.what());
        closeHttpConn(httpConn);
    }
}

void WebServer::process(std::shared_ptr<HttpConn> &httpConn) {
    assert(httpConn);
    bool processFinish = httpConn->process();

    if (processFinish) {
        // 处理成功，相当于数据读取完毕，然后将httpConn的fd加入epoller的写事件
        epoller_->modFd(httpConn->getFd(), httpConnEvent_ | EPOLLOUT);
    } else {
        // 处理失败，相当于需要读数据，则将httpConn的fd加入epoller的读事件
        epoller_->modFd(httpConn->getFd(), httpConnEvent_ | EPOLLIN);
    }
}