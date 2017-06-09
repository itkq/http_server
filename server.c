#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>

#define TCP_PORT            3000
#define LISTEN_BACKLOG      10

int main(int argc, char **argv) {
    int sfd, cfd;
    int n;
    char buf[512];
    const int on = 1;
    struct sockaddr_in addr;
    socklen_t peer_addr_size;

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    peer_addr_size = sizeof(struct sockaddr);
    cfd = accept(sfd, (struct sockaddr *) &addr, &peer_addr_size);
    if (cfd == -1) {
        perror("accept");
    }

    while (1) {
        memset(buf, 0, sizeof(buf));
        if ((n = read(cfd, buf, sizeof(buf))) > 0) {
            printf("%d %s\n", n, buf);
            write(cfd, buf, sizeof(buf));
        }
    }

    close(cfd);

    return 0;
}
