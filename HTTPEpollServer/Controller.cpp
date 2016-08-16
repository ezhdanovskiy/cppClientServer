#include "Controller.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>

#define BUFFER_SIZE_2 100

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

int epollCtlAdd(int epfd, BaseController *ptr, unsigned int events) {
    LOG(__func__ << "(ptr->getFD()=" << ptr->getFD() << ")");
    struct epoll_event e;
    e.data.ptr = ptr;
    e.events = events;
    return epoll_ctl(epfd, EPOLL_CTL_ADD, ptr->getFD(), &e);
}

BaseController::BaseController(int fd) : fd(fd), status(EventStatus::empty) {
    LOG(__func__ << "(fd=" << fd << ")");
    if (fd > 0) {
        status = EventStatus::inProcess;
    }
}

int BaseController::getFD() {
    return fd;
}

BaseController::~BaseController() {
    LOG(__func__ << "() fd=" << fd);
    if (fd) {
        close(fd);
    }
}


AcceptController::AcceptController(int fd) : BaseController(fd) {
    LOG(__func__ << "(fd=" << fd << ")");
}

BaseController::EventStatus AcceptController::dispatch(const epoll_event &event, int events_fd) {
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

AcceptController::~AcceptController() {
    LOG(__func__ << "() fd=" << fd);
}


SocketController::SocketController(int fd) : BaseController(fd) {
    LOG(__func__ << "(fd=" << fd << ")");
}

BaseController::EventStatus SocketController::dispatch(const epoll_event &event, int events_fd) {
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
            if (in.find("\r\n\r\n") == std::string::npos) {
                return EventStatus::inProcess;
            }
        }
        LOG("  in(" << in.size() << ")='\033[1m" << in << "\033[0m'");

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

SocketController::~SocketController() {
    LOG(__func__ << "() fd=" << fd);
}
