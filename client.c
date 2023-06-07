#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define MAX 80
#define PORT 41181
#define SA struct sockaddr

void *func(void *ptr)
{
    int *base_ptr = ptr;
    int *type_ptr = ptr + 4;
    int sockfd = *base_ptr;
    int type = *type_ptr;
    while (1) {
        if (type) {
            char buff[MAX];
            bzero(buff, sizeof(buff));
            buff[0] = 1;
            int c = read(STDIN_FILENO, buff + 1, 1000);
            if (c == 0) {
                send(sockfd, 0, sizeof(buff), 0);
                close(sockfd);
                exit(0);
            }
            send(sockfd, buff, sizeof(buff), 0);
        } else {
            char buff[MAX];
            bzero(buff, sizeof(buff));
            int c = read(sockfd, buff, sizeof(buff));
            if (c <= 0) {
                break;
            }
            printf("%s", buff);
        }
    }
}

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr, cli;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(PORT);
    connect(sockfd, (SA*)&servaddr, sizeof(servaddr));
    char buff[10000];
    int ptr = 0;
    int ptr_now = 0;
    for (int i = 3; i < argc; ++i) {
        ptr_now = 0;
        if (i != 0) {
            buff[ptr] = ' ';
            ++ptr;
        }
        while (1) {
            buff[ptr] = argv[i][ptr_now];
            if (buff[ptr] == '\0') {
                break;
            }
            ++ptr_now;
            ++ptr;
        }
    }
    send(sockfd, buff, 10000, 0);
    int sockfd2 = dup(sockfd);
    int a[2];
    a[0] = sockfd;
    a[1] = 0;
    pthread_t f_t;
    pthread_create(&f_t, NULL, *func, (void *)&a);
    int b[2];
    b[0] = sockfd2;
    b[1] = 1;
    pthread_t s_t;
    pthread_create(&s_t, NULL, *func, (void *)&b);
    pthread_join(f_t, NULL);
    close(sockfd);
    return 0;
}