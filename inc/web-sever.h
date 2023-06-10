#define PORT 8039
#define BUFFER_SIZE 1024
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#define handle_error(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

struct httpRequest
{
    char *method;
    char *uri;
    char *version;
    char *button;
};

int pasiveSocket(struct sockaddr *host_addr, int host_addrlen);
int childProcess(int client_fd, struct sockaddr *client_addr, socklen_t *client_addrlen,char *path);
struct httpRequest *readRequest(int sockfd);
char *processResponse(struct httpRequest *request, char *path);
int sendResponse(int sockfd, char *resp);
char *readAction(char *request);
char *createHTML(char *path);