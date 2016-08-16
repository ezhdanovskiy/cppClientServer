#include "defs.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string>
#include <string.h>


int main(int argc, char **argv) {
    int port = 22000;
    std::string host = "127.0.0.1";
    if (argc >= 3) {
        host = argv[1];
        port = atoi(argv[2]);
    }
    LOG("LISTEN " << host << ":" << port);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &(servaddr.sin_addr));

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }

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

    struct sockaddr_in client_addr;
    socklen_t ca_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &ca_len);
    LOG("accept(" << listen_fd << ") return " << client_fd);
    LOG("Client connected: " << inet_ntoa(client_addr.sin_addr));

    char str[100];
    while (1) {
        bzero(str, 100);
        if (read(client_fd, str, 100) <= 0) {
            break;
        }

        printf("Echoing back - %s", str);
        write(client_fd, str, strlen(str) + 1);
    }
}
