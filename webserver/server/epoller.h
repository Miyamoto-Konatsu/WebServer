#pragma once

#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>

class Epoller {
public:
    explicit Epoller(int eventSize = 4096);

    ~Epoller();

    // timeout: ms
    int epollWait(int timeout);

    bool addFd(int fd, uint32_t events);
    bool delFd(int fd);
    bool modFd(int fd, uint32_t events);

    struct epoll_event getEvent(int index);

private:
    int epollFd_;
    std::vector<struct epoll_event> epollEvents_;
};