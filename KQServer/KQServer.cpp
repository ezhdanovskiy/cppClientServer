#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <map>

#include <defs.h>

std::map<int, FDType> fdMap;
struct kevent events_to_monitor[MAX_EVENTS];
int events_to_monitor_size = 1;

int setNonblocking(int fd);
void kqAdd(int ident, short filter, unsigned short flags, void *const udata);
std::ostream& operator<<(std::ostream &o, const struct kevent &ev);

int main(int argc, char **argv) {
    int port = 22000;
    std::string host = "127.0.0.1";
    if (argc >= 3) {
        host = argv[1];
        port = atoi(argv[2]);
    }
    LOG("LISTEN " << host << ":" << port);

    int events_fd = kqueue();
    if (events_fd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    fdMap[events_fd] = FDType::events;

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &(servaddr.sin_addr));

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    fdMap[listen_fd] = FDType::listen;

    int enable = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        err(1, "setsockopt(SO_REUSEADDR) failed %s:%d", __FILE__, __LINE__);
    }

    if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        err(1, "Error bind \t%s:%d", __FILE__, __LINE__);
    }
    if (listen(listen_fd, 10) < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    setNonblocking(listen_fd);

    kqAdd(listen_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0);

    struct kevent events[MAX_EVENTS];
    while (1) {
        int events_size = kevent(events_fd, events_to_monitor, events_to_monitor_size, events, MAX_EVENTS, 0);
        events_to_monitor_size = 0;
        for (int i = 0; i < events_size; ++i) {
            struct kevent &event = events[i];
            LOG(event);
            int fd = (int) event.ident;
            auto fdMapIt = fdMap.find(fd);
            if (fdMapIt == fdMap.end()) {
                warn("Error accepting \t%s:%d", __FILE__, __LINE__);
                continue;
            }
            switch (fdMapIt->second) {
                case FDType::listen: {
                    struct sockaddr_in client_addr;
                    socklen_t ca_len = sizeof(client_addr);
                    int client_fd = accept(fd, (struct sockaddr *) &client_addr, &ca_len);
                    LOG("accept(" << fd << ") return " << client_fd);
                    if (client_fd < 0) {
                        warn("Error accepting \t%s:%d", __FILE__, __LINE__);
                        continue;
                    }
                    LOG("Client connected: " << inet_ntoa(client_addr.sin_addr));
                    fdMap[client_fd] = FDType::client;
                    setNonblocking(client_fd);

                    kqAdd(client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0);
                    kqAdd(client_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0);
                    break;
                }
                case FDType::client: {
                    if (event.filter == EVFILT_READ) {
                        if (event.flags & EV_EOF) {
                            LOG("close(" << fd << ")");
                            close(fd);
                            break;
                        }

                        char buffer[BUFFER_SIZE];
                        int received = recv(fd, buffer, BUFFER_SIZE, 0);
                        if (received < 0) {
                            warn("Error reading from socket \t%s:%d", __FILE__, __LINE__);
                            break;
                        }
                        if (received == 0) {
                            LOG("close(" << fd << ")");
                            close(fd);
                            break;
                        } else {
                            buffer[received] = 0;
                            LOG("Reading " << received << " bytes: '" << buffer << "'");
                        }
                        if (send(fd, buffer, received, 0) != received) {
                            warn("Could not write to stream \t%s:%d", __FILE__, __LINE__);
                            continue;
                        }
                    }
                    break;
                }
            }
        }
    }
    close(events_fd);
}

int setNonblocking(int fd) {
    int flags = 0;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void kqAdd(int ident, short filter, unsigned short flags, void *const udata) {
    if (events_to_monitor_size == MAX_EVENTS) {
        warn("To much events! \t%s:%d", __FILE__, __LINE__);
        return;
    }
    struct kevent *kep = &events_to_monitor[events_to_monitor_size++];
    kep->ident = (uintptr_t) ident;
    kep->filter = filter;
    kep->flags = flags;
    kep->fflags = 0;
    kep->data = 0;
    kep->udata = udata;
}

std::ostream& operator<<(std::ostream &o, const struct kevent &ev) {
    return o << "epoll_event{fd=" << ev.ident << " filter=" << ev.filter << " flags=" << std::hex << ev.flags << std::dec << "}";
}
