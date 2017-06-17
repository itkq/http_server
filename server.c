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

enum REQUEST_LINE_ID {
    REQ_METHOD,
    REQ_PATH,
    REQ_VERSION
} ;

const char *ACCEPTABLE_METHOD[1] = {"GET"};
const char *ACCEPTABLE_VERSION[1] = {"HTTP/1.0"};
const char *documentRoot = "./html";

int child_pids[CONCURRENCY];

int str_include(char *needle, const char **arr) {
    const char **str;
    for (str = arr; *str != NULL; str++) {
        if (strcmp(needle, *str) == 0) return 1;
    }

    return 0;
}

void handle_request(int cfd, char *req) {
    int line_no, token_no;
    char *str, *line, *token;
    char *saveptr_line, *saveptr_token;

    for (line_no = 0, str = req; ; str = NULL, line_no++) {
        line = strtok_r(str, "\r\n", &saveptr_line);
        if (line == NULL) break;

        if (line_no == 0) {
            char *tokens[3];
            char effective_path[64];
            FILE *fp;

            for (token_no = 0, token = line; ; token = NULL, token_no++) {
                token = strtok_r(token, " ", &saveptr_token);
                if (token == NULL) break;

                tokens[token_no] = token;
            }

            if (!str_include(tokens[REQ_METHOD], ACCEPTABLE_METHOD)) {
                printf("error: specified method \"%s\" cannot be accepted.", tokens[REQ_METHOD]);
                return;
            }
            if (!str_include(tokens[REQ_VERSION], ACCEPTABLE_VERSION)) {
                printf("error: specified version \"%s\" cannot be accepted.", tokens[REQ_VERSION]);
                return;
            }

            strncpy(effective_path, documentRoot, 64);
            strcat(effective_path, tokens[REQ_PATH]);

            printf("effective path: %s\n", effective_path);
            if ((fp = fopen(effective_path, "r"))) {
                char buf[256];

                while ((memset(buf, 0, sizeof(buf)), fgets(buf, sizeof(buf), fp)) != NULL) {
                    printf("%s", buf);
                    send(cfd, buf, sizeof(buf), 0);
                }

                fclose(fp);

            } else {
                perror("file open");
            }
        }
    }

}

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
        addr_len = sizeof(struct sockaddr_in);

        while (1) {
            cfd = accept(sfd, (struct sockaddr *) &addr, &addr_len);
            memset(buf, 0, sizeof(buf));

            while ((n = recv(cfd, buf, sizeof(buf), 0)) > 0) {
                printf("pid: %d, size: %d\n%s", getpid(), n, buf);
                handle_request(cfd, buf);
                close(cfd);
            }
        }
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

    // Reuse socket
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
