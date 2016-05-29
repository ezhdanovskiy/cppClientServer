#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <map>

#define LOG(chain) std::cout << chain << std::endl;

#define EPOLL_EVENTS 10
#define MAX_EVENTS 10
#define BUFFER_SIZE 1000

enum class FDType {
    epoll,
    listen,
    client
};
std::map<int, FDType> fdMap;

int setNonblocking(int fd);
int epollCtlAdd(int epfd, int listen_fd, unsigned int events);

int main() {
    int epfd = epoll_create(EPOLL_EVENTS);
    if (epfd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    fdMap[epfd] = FDType::epoll;

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(22000);
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    fdMap[listen_fd] = FDType::listen;

    if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        err(1, "");
    }
    if (listen(listen_fd, 10) < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    setNonblocking(listen_fd);

    if (epollCtlAdd(epfd, listen_fd, EPOLLIN) < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1 /* Timeout */);

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            auto fdMapIt = fdMap.find(fd);
            if (fdMapIt == fdMap.end()) {
                warn("Error accepting");
                continue;
            }
            switch (fdMapIt->second) {
                case FDType::listen: {
                    struct sockaddr_in client_addr;
                    socklen_t ca_len = sizeof(client_addr);
                    int client_fd = accept(fd, (struct sockaddr *) &client_addr, &ca_len);
                    if (client_fd < 0) {
                        warn("Error accepting");
                        continue;
                    }
                    LOG("Client connected: " << inet_ntoa(client_addr.sin_addr));
                    fdMap[client_fd] = FDType::client;
                    setNonblocking(client_fd);

                    if (epollCtlAdd(epfd, client_fd, EPOLLIN) < 0) {
                        err(1, "%s:%d", __FILE__, __LINE__);
                    }
                    break;
                }
                case FDType::client: {
                    if (events[i].events & EPOLLIN) {
                        char buffer[BUFFER_SIZE];
                        int received = recv(fd, buffer, BUFFER_SIZE, 0);
                        if (received < 0) {
                            warn("Error reading from socket");
                            continue;
                        } else {
                            buffer[received] = 0;
                            LOG("Reading " << received << " bytes: '" << buffer << "'");
                        }
                        if (send(fd, buffer, received, 0) != received) {
                            warn("Could not write to stream");
                            continue;
                        }
                    }
                    if (events[i].events & EPOLLERR) {
                        err(1, "%s:%d", __FILE__, __LINE__);
                    }
                    break;
                }
            }
        }
    }
}

int setNonblocking(int fd) {
    int flags = 0;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int epollCtlAdd(int epfd, int listen_fd, unsigned int events) {
    struct epoll_event e;
    e.data.fd = listen_fd;
    e.events = events;
    return epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &e);
}
