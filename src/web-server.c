#include "../inc/web-sever.h"

int main(int argc, char *argv[])
{
#pragma region handle_Args
    int port;
    char path[500];
    if (argc < 3)
    {
        port = PORT;
        strcpy(path, "/home");
    }
    else
    {
        port = atoi(argv[1]);
        strcpy(path, argv[2]);
    }
    printf("Port: %d\n", port);
    printf("Path: %s\n", path);
#pragma endregion
#pragma region Variables
    struct sockaddr_in host_addr;
    int host_addrlen = sizeof(host_addr);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);
#pragma endregion

    int sock_fd = pasiveSocket((struct sockaddr *)&host_addr, host_addrlen);

    while (1)
    {
        // Accept incoming connections
        int newsockfd = accept(sock_fd, (struct sockaddr *)&host_addr, (socklen_t *)&host_addrlen);
        if (newsockfd < 0)
        {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");
        int childpid = childProcess(newsockfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);
        if (childpid == -1)
        {
            printf("child fail forking");
        }
        close(newsockfd);
    }
}
int pasiveSocket(struct sockaddr *host_addr, int host_addrlen)
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd == -1)
    {
        handle_error("WebSever(socket)");
    }
    printf("socket created successfully\n");

    if (bind(sock_fd, host_addr, host_addrlen) != 0)
    {
        handle_error("webserver (bind)");
    }
    printf("socket successfully bound to address\n");

    if (listen(sock_fd, SOMAXCONN) != 0)
    {
        handle_error("webserver (listen))");
    }
    printf("server listening for connections\n");
    return sock_fd;
}
int childProcess(int sockfd, struct sockaddr *client_addr, socklen_t *client_addrlen)
{
    pid_t childPid = fork();
    if (childPid == 0)
    {
        struct httpRequest* request = readRequest(sockfd);
        
        // processResponse(request);
        // sendResponse(sockfd, resp);
    }
    return childPid;
}
struct httpRequest* readRequest(int sockfd)
{
    char buffer[BUFFER_SIZE];
    struct httpRequest *request = (struct httpRequest*) malloc(sizeof(struct httpRequest));
    // Create client address
    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);

    // Get client address
    int sockn = getsockname(sockfd, (struct sockaddr *)&client_addr,
                            (socklen_t *)&client_addrlen);
    if (sockn < 0)
    {
        perror("webserver (getsockname)");
    }

    int valread = read(sockfd, buffer, BUFFER_SIZE);
    
    if (valread < 0)
    {
        perror("webserver (read)");
    }
    // Info del Client
    printf("New Client [%s:%u]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    printf("---------------------\n");
    printf("|     Request       |\n");
    printf("---------------------\n");
    printf("%s\n",buffer);
    sscanf(buffer, "%s %s %s", request->method, request->uri, request->version);
    printf("---------------------\n");
    printf("|      method       |\n");
    printf("---------------------\n");
    printf("%s\n",request->method);
    printf("---------------------\n");
    printf("|        Uri        |\n");
    printf("---------------------\n");
    printf("%s\n",request->uri);
    printf("---------------------\n");
    printf("|      version      |\n");
    printf("---------------------\n");
    printf("%s\n",request->version);


    return request;
}