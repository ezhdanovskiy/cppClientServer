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

int epollCtlAdd(int fdEvents, BaseController *ptr, unsigned int events) {
    LOG(__func__ << "(ptr->getFD()=" << ptr->getFD() << ")");
    struct epoll_event e;
    e.data.ptr = ptr;
    e.events = events;
    return epoll_ctl(fdEvents, EPOLL_CTL_ADD, ptr->getFD(), &e);
}

int epollCtlMod(int fdEvents, BaseController *ptr, unsigned int events) {
    LOG(__func__ << "(ptr->getFD()=" << ptr->getFD() << ")");
    struct epoll_event e;
    e.data.ptr = ptr;
    e.events = events;
    return epoll_ctl(fdEvents, EPOLL_CTL_MOD, ptr->getFD(), &e);
}

BaseController::BaseController(int fd) : fdSocket(fd), status(EventStatus::empty) {
    LOG(__func__ << "(fd=" << fd << ")");
    if (fd > 0) {
        status = EventStatus::inProcess;
    }
}

int BaseController::getFD() {
    return fdSocket;
}

BaseController::~BaseController() {
    LOG(__func__ << "() fd=" << fdSocket);
    if (fdSocket) {
        LOG("  close(fd=" << fdSocket << ")");
        close(fdSocket);
    }
}


AcceptController::AcceptController(int fd) : BaseController(fd) {
    LOG(__func__ << "(fd=" << fd << ")");
}

BaseController::EventStatus AcceptController::dispatch(const epoll_event &event, int fdEvents) {
    LOG("AcceptController::" << __func__ << "(" << event << ")");
    struct sockaddr_in client_addr;
    socklen_t ca_len = sizeof(client_addr);
    int fdClient = accept(fdSocket, (struct sockaddr *) &client_addr, &ca_len);
    LOG("  accept(" << fdSocket << ") return " << fdClient);
    if (fdClient < 0) {
        warn("Error accepting \t%s:%d", __FILE__, __LINE__);
        return EventStatus::error;
    }
    LOG("  Client connected: " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port));
    setNonblocking(fdClient);

    if (epollCtlAdd(fdEvents, new SocketController(fdClient), EPOLLIN | EPOLLRDHUP) < 0) {
        LOG("ERROR");
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }
    return EventStatus::inProcess;
};

AcceptController::~AcceptController() {
    LOG(__func__ << "() fd=" << fdSocket);
}


SocketController::SocketController(int fd) : BaseController(fd) {
    LOG(__func__ << "(fd=" << fd << ")");
}

BaseController::EventStatus SocketController::dispatch(const epoll_event &event, int fdEvents) {
    LOG("SocketController::" << __func__ << "(" << event << ")");
    if (event.events & EPOLLRDHUP) {
        return EventStatus::finished;
    }

    if (event.events & EPOLLERR) {
        return EventStatus::error;
    }

    if (event.events & EPOLLIN) {
        char buffer[BUFFER_SIZE_2 + 1];
        long received = ::recv(fdSocket, buffer, BUFFER_SIZE_2, 0);
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

        out = "HTTP/1.1 200 OK\n\n";

        std::stringstream ansBody;
        ansBody << "<!DOCTYPE HTML><html><head><meta charset=\"utf-8\"><title>HttpServer</title></head><body>";
        for (long j = 1; j < 1000000; ++j) {
            ansBody << j + 1000000000 << "<br>\n";
        }
        ansBody << "</body></html>";
        out += ansBody.str();
        LOG("  out.size=" << out.size() << " bytes");
        if (epollCtlMod(fdEvents, this, EPOLLOUT | EPOLLRDHUP) < 0) {
            LOG("ERROR");
            err(1, "\t%s:%d", __FILE__, __LINE__);
        }
        return EventStatus::inProcess;
    }

    if (event.events & EPOLLOUT) {
        usleep(500000);
        long sent = ::send(fdSocket, out.c_str() + sentAll, out.size() - sentAll, 0);
        CHECK_LONG_ERR(sent);
        sentAll += sent;
        LOG("  send " << sentAll << " bytes of " << out.size() << " bytes");
        if (sentAll < out.size()) {
            return EventStatus::inProcess;
        }
    }

    LOG("  close(fd=" << fdSocket << ")");
    close(fdSocket);
    return EventStatus::finished;
};

SocketController::~SocketController() {
    LOG(__func__ << "() fd=" << fdSocket);
}
