#include "Controller.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>

int setNonblocking(int fd) {
    LOG(__func__ << "(fd=" << fd << ")");
    int flags = 0;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

std::ostream &operator<<(std::ostream &o, const epoll_event &event) {
    BaseController *controller = (BaseController *) event.data.ptr;
    return o << "epoll_event{fd=" << controller->getFD() << std::hex << " events=" << event.events << " ptr=" << event.data.ptr << std::dec << "}";
}

int epollCtlAdd(int fdEvents, BaseController *ptr, unsigned int events) {
    LOG(__func__ << "(fd=" << ptr->getFD() << ")");
    struct epoll_event e;
    e.data.ptr = ptr;
    e.events = events;
    return epoll_ctl(fdEvents, EPOLL_CTL_ADD, ptr->getFD(), &e);
}

int epollCtlMod(int fdEvents, BaseController *ptr, unsigned int events) {
    LOG(__func__ << "(fd=" << ptr->getFD() << ")");
    struct epoll_event e;
    e.data.ptr = ptr;
    e.events = events;
    return epoll_ctl(fdEvents, EPOLL_CTL_MOD, ptr->getFD(), &e);
}

BaseController::BaseController(int fd) : fdSocket(fd) {
    LOG(__func__ << "(fd=" << fd << ")");
}

int BaseController::getFD() {
    return fdSocket;
}

BaseController::~BaseController() {
    LOG(__func__ << "() fd=" << fdSocket);
    if (fdSocket) {
        LOG("  close(fd=" << fdSocket << ")");
        close(fdSocket);
        fdSocket = 0;
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

    SocketController *socketController = new SocketController(fdClient);
    LOG("  new SocketController(" << std::hex << socketController << std::dec << ")");
    if (epollCtlAdd(fdEvents, socketController, EPOLLIN | EPOLLRDHUP) < 0) {
        LOG("ERROR");
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }
    return EventStatus::inProcess;
};

AcceptController::~AcceptController() {
    LOG(__func__ << "() fd=" << fdSocket);
}


SocketController::SocketController(int fd) : BaseController(fd), sentAll(0) {
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
        char buffer[BUFFER_SIZE + 1];
        long received = ::recv(fdSocket, buffer, BUFFER_SIZE, 0);
        LOG1(received);
        if (received < 0) {
            warn("Error reading from socket \t%s:%d", __FILE__, __LINE__);
            return EventStatus::error;
        }

        if (received > 0) {
            buffer[received] = 0;
            in.append(buffer);
            LOG("  Read " << received << " bytes: '\033[1m" << buffer << "\033[0m' from fd=" << fdSocket);
            if (in.find("\r\n\r\n") == std::string::npos) {
                return EventStatus::inProcess;
            }
        }
//        LOG("  in(" << in.size() << ")='\033[1m" << in << "\033[0m'");

//        {
//            out = "HTTP/1.1 200 OK\n\n";
//            std::stringstream ansBody;
//            ansBody << "<!DOCTYPE HTML><html><head><meta charset=\"utf-8\"><title>HttpServer</title></head><body>";
//            for (long j = 1; j < 100000; ++j) {
//                ansBody << j + 1000000000 << "<br>\n";
//            }
//            ansBody << "</body></html>";
//            LOG("  ansBody.size=" << ansBody.str().size() << " bytes");
//            out += ansBody.str();
//        }
        out =   "HTTP/1.1 200 OK\n"
                "\n"
                "<!DOCTYPE HTML>\n"
                "<html>\n"
                "  <head>\n"
                "    <meta charset=\"utf-8\">\n"
                "    <title>HttpServer</title>\n"
                "  </head>\n"
                "  <body>\n"
                "    Hello world!\n"
                "  </body>\n"
                "</html>\n";


        LOG("  out.size=" << out.size() << " bytes");
        if (epollCtlMod(fdEvents, this, EPOLLOUT | EPOLLRDHUP) < 0) {
            LOG("ERROR");
            err(1, "\t%s:%d", __FILE__, __LINE__);
        }
        return EventStatus::inProcess;
    }

    if (event.events & EPOLLOUT) {
        long sent = ::send(fdSocket, out.c_str() + sentAll, out.size() - sentAll, 0);
        CHECK_LONG_ERR(sent);
        sentAll += sent;
        LOG("  send " << sentAll << " bytes of " << out.size() << " bytes to fd=" << fdSocket);
        if (sentAll < out.size()) {
//            usleep(100000);
            return EventStatus::inProcess;
        }
    }

    return EventStatus::finished;
};

SocketController::~SocketController() {
    LOG(__func__ << "() fd=" << fdSocket);
}
