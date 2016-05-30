#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include <defs.h>

int main(int argc, char **argv) {
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(22000);
    inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    LOG("connect(" << sockfd << ")");

    char sendline[100];
    char recvline[100];
    char quit[] = "quit";
    while (1) {
        bzero(sendline, 100);
        bzero(recvline, 100);
        fgets(sendline, 100, stdin); /*stdin = 0 , for standard input */
        if (strlen(sendline) >= strlen(quit) &&  strncmp(sendline, quit, strlen(quit)) == 0 ) {
            LOG("close(" << sockfd << ")");
            close(sockfd);
            break;
        }

        LOG("write(" << sockfd << ", '" << sendline << "')");
        write(sockfd, sendline, strlen(sendline) + 1);

        read(sockfd, recvline, 100);
        LOG("read(" << sockfd << ", '" << recvline << "')");
    }
}
