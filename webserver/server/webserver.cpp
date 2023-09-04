#include "webserver.h"
#include "http/httpconn.h"
#include "log/log.h"
#include "pool/threadpool.h"
#include "server/epoller.h"
#include "timer/heaptimer.h"
#include "sys/socket.h"
#include "unistd.h"
#include <cassert>
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>

static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

WebServer::WebServer(const char *ip, int port, int triggerMode, int timeout) :
    ip_(ip), port_(port), triggerMode_(triggerMode), httpTimeOut_(timeout),
    threadPool_(new ThreadPool), timer_(new HeapTimer), epoller_(new Epoller) {
    init();
}

WebServer::~WebServer() {
}

void WebServer::init() {
    Log::getInstance()->init(0);
    threadPool_->start();

    initTriggerMode();

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
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
}

void WebServer::start() {
    int epollTimeOut = -1;
    while (!isClosed_) {
        if (httpTimeOut_ > 0) { epollTimeOut = timer_->getNextTick(); }
        int eventNum = epoller_->epollWait(epollTimeOut);
        if (eventNum < 0) {
            LOG_ERROR("epoll_wait error");
            break;
        }
        for (int i = 0; i < eventNum; ++i) {
            int fd = epoller_->getEvent(i).data.fd;
            if (fd == listenFd_) {
                dealListen();
            } else if (epoller_->getEvent(i).events & (EPOLLRDHUP | EPOLLERR)) {
                // todo:现在遇到EPOLLRDHUP和EPOLLERR都是直接关闭连接，后续应该单独处理EPOLLRDHUP事件
                assert(httpConnMap_.count(fd) > 0);
                httpConnMap_.erase(fd);
                assert(httpConnTimerMap_.count(fd) > 0);
                timer_->delTimerNode(httpConnTimerMap_[fd]);
                httpConnTimerMap_.erase(fd);
                close(fd);
            } else if (epoller_->getEvent(i).events & EPOLLIN) {
                assert(httpConnMap_.count(fd) > 0);
                dealRead(httpConnMap_[fd]);
            } else if (epoller_->getEvent(i).events & EPOLLOUT) {
                assert(httpConnMap_.count(fd) > 0);
                dealWrite(httpConnMap_[fd]);
            }
        }
    }
}

void WebServer::closeHttpConn(int httpClientFd) {
    assert(httpConnTimerMap_.count(httpClientFd) > 0);
    assert(httpConnMap_.count(httpClientFd) > 0);

    LOG_ERROR("close httpConn, fd: %d", httpClientFd);
    timer_->delTimerNode(httpConnTimerMap_[httpClientFd]);
    httpConnTimerMap_.erase(httpClientFd);

    httpConnMap_[httpClientFd]->close();
    httpConnMap_.erase(httpClientFd);
}

void WebServer::dealListen() {
    do {
        struct sockaddr_in addr;
        socklen_t addr_len;
        int httpClientFd =
            accept(listenFd_, (struct sockaddr *)&addr, &addr_len);
        char clientIp[INET_ADDRSTRLEN];
        short clientPort;

        // 使用ntohs函数将端口从网络字节顺序转换为主机字节顺序
        clientPort = ntohs(addr.sin_port);

        // 使用inet_ntop函数将IP地址从网络字节顺序转换为字符串表示形式
        if (inet_ntop(AF_INET, &(addr.sin_addr), clientIp, INET_ADDRSTRLEN)
            == NULL) {
            LOG_WARN("inet_ntop error")
            close(httpClientFd);
            continue;
        } else {
            LOG_INFO("accept a connection from %s:%d\n", clientIp, clientPort);
        }

        httpConnMap_[httpClientFd] =
            std::make_shared<HttpConn>(httpClientFd, clientIp, clientPort);
        if (httpTimeOut_ > 0) {
            TimerId timerId = timer_->add(
                [this, httpClientFd]() { closeHttpConn(httpClientFd); },
                this->httpTimeOut_ / 1000, this->httpTimeOut_ % 1000);
        }

        setNonBlocking(httpClientFd);
        epoller_->addFd(httpClientFd, httpConnEvent_ | EPOLLIN);

    } while (listenEvent_ | EPOLLET);
}

void WebServer::dealRead(std::shared_ptr<HttpConn> &httpConn) {
    assert(httpConn);
    threadPool_->addTask([this, httpConn]() { onRead(httpConn); });
    resetTimeout(httpConn);
}

void WebServer::dealWrite(std::shared_ptr<HttpConn> &httpConn) {
    assert(httpConn);
    threadPool_->addTask([this, httpConn]() { onWrite(httpConn); });
    resetTimeout(httpConn);
}

void WebServer::resetTimeout(std::shared_ptr<HttpConn> &httpConn) {
    assert(httpConn);
    if (httpTimeOut_ > 0) {
        TimerId timerId = httpConnTimerMap_[httpConn->getFd()];
        timer_->adjust(timerId, httpTimeOut_ / 1000, httpTimeOut_ % 1000);
    }
}

void WebServer::onRead(std::shared_ptr<HttpConn> httpConn) {
    assert(httpConn);
    int readErrno = 0;
    int readSize = httpConn->read(readErrno);
    if (readSize < 0 && readErrno != EAGAIN || readSize == 0) {
        closeHttpConn(httpConn->getFd());
        return;
    }
    LOG_INFO("read from httpconn, fd: %d, readSize: %d", httpConn->getFd(),
             readSize);
    process(httpConn);
}

void WebServer::onWrite(std::shared_ptr<HttpConn> httpConn) {
    assert(httpConn);
    int writeErrno = 0;
    int writeSize = httpConn->write(writeErrno);
    if (writeSize < 0 && writeErrno != EAGAIN) {
        closeHttpConn(httpConn->getFd());
        return;
    }
    LOG_INFO("write to httpconn, fd: %d, writeSize: %d", httpConn->getFd(),
             writeSize);
    if (httpConn->needWrite()) {
        epoller_->modFd(httpConn->getFd(), httpConnEvent_ | EPOLLOUT);
    } else {
        if (httpConn->isKeepAlive()) {
            process(httpConn);
            return;
        }
        closeHttpConn(httpConn->getFd());
    }
}

void WebServer::process(std::shared_ptr<HttpConn> &httpConn) {
    assert(httpConn);
    if (httpConn->process()) {
        // 处理成功，相当于数据读取完毕，然后将httpConn的fd加入epoller的写事件
        epoller_->modFd(httpConn->getFd(), httpConnEvent_ | EPOLLOUT);
    } else {
        // 处理失败，相当于需要读数据，则将httpConn的fd加入epoller的读事件
        epoller_->modFd(httpConn->getFd(), httpConnEvent_ | EPOLLIN);
    }
}