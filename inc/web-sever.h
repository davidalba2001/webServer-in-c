#define PORT 6969
#define BUFFER_SIZE 1024
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#define handle_error(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

struct httpRequest
{
    char method[10];
    char *uri;
    char version[10];
};

int pasiveSocket(struct sockaddr *host_addr, int host_addrlen);
int childProcess(int sockfd, struct sockaddr *client_addr, socklen_t *client_addrlen);
struct httpRequest *readRequest(int sockfd);
