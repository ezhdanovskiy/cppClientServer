#include "defs.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
    LOG("connect to " << host << ":" << port << " argc=" << argc);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &(servaddr.sin_addr));

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    LOG("create socket " << sock_fd);

    if (connect(sock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        err(1, "%s:%d", __FILE__, __LINE__);
    }
    LOG("connect(" << sock_fd << ")");

    char sendline[100];
    char recvline[100];
    char quit[] = "quit";
    while (1) {
        bzero(sendline, 100);
        bzero(recvline, 100);
        fgets(sendline, 100, stdin); /*stdin = 0 , for standard input */
        if (strlen(sendline) >= strlen(quit) &&  strncmp(sendline, quit, strlen(quit)) == 0 ) {
            LOG("close(" << sock_fd << ")");
            close(sock_fd);
            break;
        }

        LOG("write(" << sock_fd << ", '" << sendline << "')");
        write(sock_fd, sendline, strlen(sendline) + 1);

        read(sock_fd, recvline, 100);
        LOG("read(" << sock_fd << ", '" << recvline << "')");
    }
}
