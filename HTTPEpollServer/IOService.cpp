#include "IOService.h"

#include "defs.h"
#include "Controller.h"

#include <string.h>
#include <memory>

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

IOService::IOService() : fdEvents(epoll_create1(0)) {
    if (fdEvents == -1) {
        err(1, "kqueue create error\t%s:%d", __FILE__, __LINE__);
    }
    LOG(__func__ << "() fdEvents=" << fdEvents);
}

IOService::~IOService() {
    if (fdEvents) {
        LOG("  close(fdEvents=" << fdEvents << ")");
        close(fdEvents);
    }
}

int IOService::run(std::string host, int port) {
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &(servaddr.sin_addr));

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }

    int enable = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        err(1, "setsockopt(SO_REUSEADDR) failed %s:%d", __FILE__, __LINE__);
    }

    if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        err(1, "Error bind \t%s:%d", __FILE__, __LINE__);
    }
    if (listen(listen_fd, 10) < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }
    setNonblocking(listen_fd);

    std::unique_ptr<AcceptController> acceptController(new AcceptController(listen_fd));
    if (epollCtlAdd(fdEvents, acceptController.get(), EPOLLIN | EPOLLRDHUP) < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }

    struct epoll_event events[MAX_EVENTS];
    int iterations = 16;
    while (iterations--) {
        LOG("epoll_wait(" << fdEvents << ")");
        int events_size = epoll_wait(fdEvents, events, MAX_EVENTS, -1 /* Timeout */);

        for (int i = 0; i < events_size; ++i) {
            const epoll_event &event = events[i];
            LOG(event);
            BaseController *controller = (BaseController *) event.data.ptr;
            BaseController::EventStatus eventStatus = controller->dispatch(event, fdEvents);
            switch (eventStatus) {
                case BaseController::EventStatus::finished: {
                    LOG("delete " << std::hex << controller << std::dec);
                    delete controller;
                    break;
                }
                case BaseController::EventStatus::error: {
                    LOG("delete " << std::hex << controller << std::dec);
                    delete controller;
                    break;
                }
            }
        }
    }

    return 0;
}
