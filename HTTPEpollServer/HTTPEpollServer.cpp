#include "defs.h"
#include "Controller.h"
#include "HTTPEpollServer.h"

#include <sys/epoll.h>
#include <string.h>
#include <err.h>
#include <map>
#include <memory>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int HTTPServer::start(int argc, char **argv) {
    int port = 22000;
    std::string host = "127.0.0.1";
    if (argc >= 3) {
        host = argv[1];
        port = atoi(argv[2]);
    }
    LOG("LISTEN " << host << ":" << port);

    events_fd = epoll_create(MAX_EVENTS);
    if (events_fd < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }

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
    if (epollCtlAdd(events_fd, acceptController.get(), EPOLLIN | EPOLLRDHUP) < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }

    struct epoll_event events[MAX_EVENTS];
    int iterations = 16;
    while (iterations--) {
        LOG("epoll_wait(" << events_fd << ")");
        int events_size = epoll_wait(events_fd, events, MAX_EVENTS, -1 /* Timeout */);

        for (int i = 0; i < events_size; ++i) {
            const epoll_event &event = events[i];
            LOG(event);
            BaseController *controller = (BaseController *) event.data.ptr;
            BaseController::EventStatus eventStatus = controller->dispatch(event, events_fd);
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
    close(events_fd);
}
