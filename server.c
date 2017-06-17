#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>

#define TCP_PORT            8000
#define LISTEN_BACKLOG      10
#define CONCURRENCY         4

int child_pids[CONCURRENCY];

int spawn_child(int sfd, struct sockaddr_in addr) {
    int cfd;
    int pid;
    int addr_len;
    char buf[512];
    int n;

    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // child
        addr_len = sizeof(struct sockaddr_in);
        // new connection
        while (1) {
            cfd = accept(sfd, (struct sockaddr *) &addr, &addr_len);

            memset(buf, 0, sizeof(buf));

            // respond to request
            while ((n = recv(cfd, buf, sizeof(buf), 0)) > 0) {
                printf("(pid: %d) %d %s", getpid(), n, buf);
                n = write(cfd, buf, sizeof(buf));
            }
        }
    } else {
        printf("spawn: %d\n", pid);
    }


    return pid;
}

int listen_tcp_server(struct sockaddr_in addr) {
    int sfd;
    const int on = 1;

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("listening on port %d\n", TCP_PORT);

    return sfd;
}

int main(int argc, char **argv) {
    int sfd;
    int i;
    pid_t pid, child_pid;
    struct sockaddr_in addr;
    struct sigaction sa;
    int status;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    pid = getpid();
    printf("main pid: %d\n", pid);
    sfd = listen_tcp_server(addr);

    for (i = 0; i < CONCURRENCY; i++) {
        child_pids[i] = spawn_child(sfd, addr);
    }

    if (getpid() == pid) {
        for (i = 0; i < CONCURRENCY; i++) {
            printf("child %d: %d\n", i, child_pids[i]);
        }
    }

    while (1) {
        wait(&status);
    }

    printf("exit server\n");
    close(sfd);

    return 0;
}
