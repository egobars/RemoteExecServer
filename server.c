#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define SA struct sockaddr

int csock, sockfd;

void sigchld_handler(int sig) {
    close(csock);
    close(sockfd);
    exit(0);
}

int main() {
    daemon(1, 1);
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(41181);
    bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(sockfd, 5);
    while (1) {
        csock = accept(sockfd, NULL, NULL);
        pid_t p = fork();
        if (p) {
            close(csock);
        } else {
            struct sigaction sa1 = {.sa_handler = sigchld_handler,
                    .sa_flags = 0};
            sigaction(SIGCHLD, &sa1, NULL);
            char now_buff[10000];
            recv(csock, now_buff, 10000, 0);
            char args[100][1000];
            int args_ptr = 0;
            int args_i_ptr = 0;
            int ptr = 0;
            int now_space = 1;
            while (1) {
                if (now_buff[ptr] == '\0') {
                    if (!now_space) {
                        args_i_ptr = 0;
                        ++args_ptr;
                    }
                    break;
                }
                if (isspace(now_buff[ptr])) {
                    if (now_space) {
                        ++ptr;
                        continue;
                    }
                    now_space = 1;
                    args[args_ptr][args_i_ptr] = '\0';
                    args_i_ptr = 0;
                    ++args_ptr;
                    ++ptr;
                    continue;
                }
                if (now_space == 1) {
                    now_space = 0;
                }
                args[args_ptr][args_i_ptr] = now_buff[ptr];
                ++args_i_ptr;
                ++ptr;
            }
            char *args_links[1000];
            for (int i = 0; i < args_ptr; ++i) {
                args_links[i] = args[i];
            }
            args_links[args_ptr] = NULL;
            int fd[2];
            pipe(fd);
            pid_t cpid = fork();
            if (cpid) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                while (1) {
                    char buff[1000];
                    bzero(buff, 1000);
                    recv(csock, buff, 1000, 0);
                    if (buff[0] == 0) {
                        close(csock);
                        close(sockfd);
                        kill(cpid, SIGTERM);
                        return 0;
                    }
                    int c = printf("%s", buff + 1);
                    if (c <= 0) {
                        break;
                    }
                }
                close(csock);
            } else {
                close(fd[1]);
                dup2(csock, STDOUT_FILENO);
                dup2(fd[0], STDIN_FILENO);
                close(csock);
                close(fd[0]);
                execvp(args_links[0], args_links);
            }
        }
    }
    close(sockfd);
    return 0;
}