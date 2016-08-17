#pragma once

#include "defs.h"
#include "Logger.h"

#include <sys/epoll.h>

#include <string>

class BaseController {
public:
    enum EventStatus {
        inProcess,
        finished,
        error
    };

    BaseController(int fd);

    int getFD();

    virtual EventStatus dispatch(const epoll_event &event, int fdEvents) = 0;

    virtual ~BaseController();

protected:
    int fdSocket;
};

class AcceptController : public BaseController {
public:
    AcceptController(int fd);

    EventStatus dispatch(const epoll_event &event, int fdEvents) override;

    virtual ~AcceptController();
};

class SocketController : public BaseController {
    std::string in;
    std::string out;
    long sentAll;
public:
    SocketController(int fd);

    EventStatus dispatch(const epoll_event &event, int fdEvents) override;

    virtual ~SocketController();
};

int setNonblocking(int fd);

int epollCtlAdd(int fdEvents, BaseController *ptr, unsigned int events);
int epollCtlMod(int fdEvents, BaseController *ptr, unsigned int events);

std::ostream &operator<<(std::ostream &o, const epoll_event &event);
