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

std::map<int, FDType> fdMap;

int setNonblocking(int fd);
int epollCtlAdd(int epfd, int listen_fd, unsigned int events);
std::ostream& operator<<(std::ostream &o, const epoll_event &ev);

int main() {
    int events_fd = epoll_create(MAX_EVENTS);
    if (events_fd < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }
    fdMap[events_fd] = FDType::events;

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(22000);
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }
    fdMap[listen_fd] = FDType::listen;

    if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        err(1, "Error bind \t%s:%d", __FILE__, __LINE__);
    }
    if (listen(listen_fd, 10) < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }
    setNonblocking(listen_fd);

    if (epollCtlAdd(events_fd, listen_fd, EPOLLIN|EPOLLRDHUP) < 0) {
        err(1, "\t%s:%d", __FILE__, __LINE__);
    }

    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int events_size = epoll_wait(events_fd, events, MAX_EVENTS, -1 /* Timeout */);

        for (int i = 0; i < events_size; ++i) {
            const epoll_event &event = events[i];
            LOG(event);
            int fd = event.data.fd;
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

                    if (epollCtlAdd(events_fd, client_fd, EPOLLIN|EPOLLRDHUP) < 0) {
                        err(1, "\t%s:%d", __FILE__, __LINE__);
                    }
                    break;
                }
                case FDType::client: {
                    if(event.events & EPOLLRDHUP) {
                        LOG("close(" << fd << ")");
                        close(fd);
                        break;
                    }
                    if (event.events & EPOLLIN) {
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
                    if (event.events & EPOLLERR) {
                        err(1, "\t%s:%d", __FILE__, __LINE__);
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

int epollCtlAdd(int epfd, int listen_fd, unsigned int events) {
    struct epoll_event e;
    e.data.fd = listen_fd;
    e.events = events;
    return epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &e);
}

std::ostream& operator<<(std::ostream &o, const epoll_event &ev) {
    return o << "epoll_event{data.fd=" << ev.data.fd << " events=" << std::hex << ev.events << std::dec << "}";
}