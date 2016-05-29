#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

int main() {
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(22000);
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    listen(listen_fd, 10);

    int comm_fd = accept(listen_fd, (struct sockaddr *) NULL, NULL);

    char str[100];
    while (1) {
        bzero(str, 100);
        read(comm_fd, str, 100);

        printf("Echoing back - %s", str);
        write(comm_fd, str, strlen(str) + 1);
    }
}
