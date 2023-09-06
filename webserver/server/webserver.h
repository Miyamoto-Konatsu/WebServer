#pragma once

#include "epoller.h"
#include <cstdint>
#include <string>
#include <sys/types.h>
#include "log/log.h"
#include "pool/threadpool.h"
#include "timer/heaptimer.h"
#include <memory>
#include <http/httpconn.h>
#include <vector>

class WebServer {
public:
    WebServer(const char *ip, int port, int triggerMode, int timeout,
              std::string srcPath);
    ~WebServer();

    void start();

private:
    void init();

    void initTriggerMode();

    void resetTimeout(std::shared_ptr<HttpConn> &);
    void onRead(std::shared_ptr<HttpConn>);
    void onWrite(std::shared_ptr<HttpConn>);

    void dealRead(std::shared_ptr<HttpConn> &);
    void dealWrite(std::shared_ptr<HttpConn> &);
    void dealListen();

    void closeHttpConn(std::weak_ptr<HttpConn>);

    void process(std::shared_ptr<HttpConn> &);

public:
    static volatile int isClosed_;

private:
    uint32_t httpConnEvent_;
    uint32_t listenEvent_;

    const char *ip_;
    int listenFd_;
    int port_;
    int httpTimeOut_; // ms
    int triggerMode_;
    std::string srcPath_;

    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<HeapTimer> timer_;
    std::shared_ptr<Epoller> epoller_;

    std::vector<std::shared_ptr<HttpConn>> httpConnMap_;
    std::vector<TimerId> httpConnTimerMap_;
};