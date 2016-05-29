#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(22000);
    inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    char sendline[100];
    char recvline[100];
    while (1) {
        bzero(sendline, 100);
        bzero(recvline, 100);
        fgets(sendline, 100, stdin); /*stdin = 0 , for standard input */

        write(sockfd, sendline, strlen(sendline) + 1);

        read(sockfd, recvline, 100);
        printf("%s", recvline);
    }
}
