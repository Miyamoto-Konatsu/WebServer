#include "epoller.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

Epoller::Epoller(int eventSize) {
    epollFd_ = epoll_create(1);
    epollEvents_.resize(eventSize);
}

Epoller::~Epoller() {
    close(epollFd_);
}

bool Epoller::addFd(int fd, uint32_t events) {
    struct epoll_event ee;
    memset(&ee, 0, sizeof(ee));
    ee.data.fd = fd;
    ee.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ee);
}

bool Epoller::delFd(int fd) {
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
}

bool Epoller::modFd(int fd, uint32_t events) {
    struct epoll_event ee;
    memset(&ee, 0, sizeof(ee));
    ee.data.fd = fd;
    ee.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ee);
}

int Epoller::epollWait(int timeout) {
    return epoll_wait(epollFd_, epollEvents_.data(), epollEvents_.size(),
                      timeout);
}

struct epoll_event Epoller::getEvent(int index) {
    return epollEvents_[index];
}