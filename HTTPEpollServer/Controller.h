#pragma once

#include "defs.h"
#include "Logger.h"

#include <sys/epoll.h>

#include <string>

class BaseController {
public:
    enum EventStatus {
        empty,
        inProcess,
        finished,
        error
    };

    BaseController(int fd);

    int getFD();

    virtual EventStatus dispatch(const epoll_event &event, int events_fd) = 0;

    virtual ~BaseController();

protected:
    int fd;
    EventStatus status;
};

class SocketController : public BaseController {
    std::string in;

public:
    SocketController(int fd);

    EventStatus dispatch(const epoll_event &event, int events_fd) override;

    virtual ~SocketController();
};

class AcceptController : public BaseController {
public:
    AcceptController(int fd);

    EventStatus dispatch(const epoll_event &event, int events_fd) override;

    virtual ~AcceptController();
};

int setNonblocking(int fd);

int epollCtlAdd(int epfd, BaseController *ptr, unsigned int events);

std::ostream &operator<<(std::ostream &o, const epoll_event &ev);
