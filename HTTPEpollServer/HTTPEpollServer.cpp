#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <map>

#include <defs.h>
#include <sstream>
#include <memory>

int events_fd;
#define BUFFER_SIZE_2 10

int setNonblocking(int fd) {
    LOG(__func__ << "(fd=" << fd << ")");
    int flags = 0;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

std::ostream &operator<<(std::ostream &o, const epoll_event &ev) {
    return o << "epoll_event{ptr=" << std::hex << ev.data.ptr << " events=" << ev.events << std::dec << "}";
}

class BaseController {
public:
    enum EventStatus {
        empty,
        inProcess,
        finished,
        error
    };

    BaseController(int fd) : fd(fd), status(EventStatus::empty) {
        LOG(__func__ << "(fd=" << fd << ")");
        if (fd > 0) {
            status = EventStatus::inProcess;
        }
    }

    int getFD() {
        return fd;
    }

    virtual EventStatus dispatch(const epoll_event &event) = 0;

    virtual ~BaseController() {
        LOG(__func__ << "() fd=" << fd);
        if (fd) {
            close(fd);
        }
    }

protected:
    int fd;
    EventStatus status;
};

int epollCtlAdd(int epfd, BaseController *ptr, unsigned int events) {
    LOG(__func__ << "(ptr->getFD()=" << ptr->getFD() << ")");
    struct epoll_event e;
    e.data.ptr = ptr;
    e.events = events;
    return epoll_ctl(epfd, EPOLL_CTL_ADD, ptr->getFD(), &e);
}

class SocketController : public BaseController {
    std::string in;

public:
    SocketController(int fd) : BaseController(fd) {
        LOG(__func__ << "(fd=" << fd << ")");
    }

    EventStatus dispatch(const epoll_event &event) override {
        LOG("SocketController::" << __func__ << "(" << event << ")");
        if (event.events & EPOLLRDHUP) {
            LOG("  close(fd=" << fd << ")");
            close(fd);
            return EventStatus::finished;
        }
        if (event.events & EPOLLERR) {
            return EventStatus::error;
        }
        if (event.events & EPOLLIN) {
            char buffer[BUFFER_SIZE_2 + 1];
            long received = recv(fd, buffer, BUFFER_SIZE_2, 0);
            LOG1(received);
            if (received < 0) {
                warn("Error reading from socket \t%s:%d", __FILE__, __LINE__);
                return EventStatus::error;
            }

            if (received > 0) {
                buffer[received] = 0;
                in.append(buffer);
                LOG("  Read " << received << " bytes: '\033[1m" << buffer << "\033[0m'");
                LOG("  in='\033[1m" << in << "\033[0m'");
                if (in.find("\r\n\r\n") == std::string::npos) {
                    return EventStatus::inProcess;
                }
            }

            std::string ansHeader = "HTTP/1.1 200 OK\n\n";
            long sentAll = send(fd, ansHeader.c_str(), ansHeader.size(), 0);
            LOG("  send " << sentAll << " bytes of " << ansHeader.size() << ": '\033[1m" << ansHeader << "\033[0m'");

            std::stringstream ansBody;
            ansBody << "<!DOCTYPE HTML><html><head><meta charset=\"utf-8\"><title>HTTPEpollServer</title></head><body>";
            for (long j = 1; j < 1000000; ++j) {
                ansBody << j + 1000000000 << "<br>\n";
            }
            ansBody << "</body></html>";

            long sentBody = 0;
            while (sentBody < ansBody.str().size()) {
                long sent = send(fd, ansBody.str().c_str() + sentBody, ansBody.str().size() - sentBody, 0);
                CHECK_LONG_ERR(sent);
                sentBody += sent;
                sentAll += sent;
                LOG("  send " << sentAll << " bytes of " << ansBody.str().size() + ansHeader.size() << " bytes");
//                usleep(500000);
            }

            LOG("  close(fd=" << fd << ")");
            close(fd);
            return EventStatus::finished;
        }
        return EventStatus::finished;
    };

    virtual ~SocketController() {
        LOG(__func__ << "() fd=" << fd);
    }
};

class AcceptController : public BaseController {
public:
    AcceptController(int fd) : BaseController(fd) {
        LOG(__func__ << "(fd=" << fd << ")");
    }

    EventStatus dispatch(const epoll_event &event) override {
        LOG("AcceptController::" << __func__ << "(" << event << ")");
        struct sockaddr_in client_addr;
        socklen_t ca_len = sizeof(client_addr);
        int client_fd = accept(fd, (struct sockaddr *) &client_addr, &ca_len);
        LOG("  accept(" << fd << ") return " << client_fd);
        if (client_fd < 0) {
            warn("Error accepting \t%s:%d", __FILE__, __LINE__);
            return EventStatus::error;
        }
        LOG("  Client connected: " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
        setNonblocking(client_fd);

        if (epollCtlAdd(events_fd, new SocketController(client_fd), EPOLLIN | EPOLLRDHUP) < 0) {
            LOG("ERROR");
            err(1, "\t%s:%d", __FILE__, __LINE__);
        }
        return EventStatus::inProcess;
    };

    virtual ~AcceptController() {
        LOG(__func__ << "() fd=" << fd);
    }
};

int main(int argc, char **argv) {
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
            BaseController::EventStatus eventStatus = controller->dispatch(event);
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
