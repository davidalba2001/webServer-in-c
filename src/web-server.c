#include "../inc/web-sever.h"
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>

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

        int childpid = childProcess(sock_fd,newsockfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen, argv[2]);
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
int childProcess(int sock_fd, int client_fd, struct sockaddr *client_addr, socklen_t *client_addrlen, char *path)
{
    pid_t childPid = fork();
    if (childPid == 0)
    {   
        struct httpRequest *request = readRequest(client_fd);
        // printf("recok");
        char *response = processResponse(request, path);
        sendResponse(client_fd, response);
    }
    return childPid;
}
struct httpRequest *readRequest(int sockfd)
{
    char buffer[BUFFER_SIZE];
    struct httpRequest *request = (struct httpRequest *)malloc(sizeof(struct httpRequest));
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
    // printf("request %s\n", buffer);
    char *method = (char *)malloc(BUFFER_SIZE);
    char *uri = (char *)malloc(BUFFER_SIZE);
    char *version = (char *)malloc(BUFFER_SIZE);
    sscanf(buffer, "%s %s %s", method, uri, version);
    request->method = method;
    request->uri = uri;
    request->version = version;

    // printf("Method:%s\n",request->method);
    // printf("Uri %s\n", request->uri);
    // printf("Version:%s\n", request->version);
    return request;
}

char *processResponse(struct httpRequest *request, char *path)
{
    char *buffer;
    if (strcmp(request->method, "GET") == 0)
    {
        // printf("Procces GET\n");
        // printf("Uri %s\n", request->uri);
        // printf("Version:%s\n", request->version);
        buffer = createHTML("/home");
    }
    else if (strcmp(request->method, "POST") == 0)
    {
        buffer = createHTML("/home");
    }
    else
    {
        buffer = createHTML("/home");
    }
    return buffer;
}

char *parseURI(char *URI)
{
}
char *generateTable(char *path)
{ // TODO: revisar algunas comprobaciones
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    sprintf(buffer, "<table>");
    strcat(buffer, "<tr><th>Name</th><th>Size</th><th>Date</th></tr>");

    DIR *dir;
    struct dirent *ent;
    dir = opendir(path);

    while ((ent = readdir(dir)) != NULL)
    {
        char *filepath = (char *)malloc(BUFFER_SIZE);
        sprintf(filepath, path);
        strcat(filepath, "/");
        strcat(filepath, ent->d_name);
        // printf("filepath %s\n", filepath);
        strcat(buffer, "<tr>");

        for (size_t j = 0; j < 3; j++)
        {
            // printf("Entto \" Forr %d\n", j);

            struct stat fileInf;
            stat(filepath, &fileInf);

            if (j == 0)
            {
                strcat(buffer, "<td>");
                strcat(buffer, "<a href=\"");
                strcat(buffer, filepath);
                strcat(buffer, "\">");
                strcat(buffer, ent->d_name);
                strcat(buffer, "</a>");
                strcat(buffer, "</td>");
            }
            if (j == 1)
            {
                strcat(buffer, "<td>");
                sprintf(buffer + strlen(buffer), "%ld", fileInf.st_size);
                strcat(buffer, "</td>");
            }
            if (j == 2)
            {
                strcat(buffer, "<td>");
                sprintf(buffer + strlen(buffer), "%s", ctime(&fileInf.st_mtime));
                strcat(buffer, "</td>");
            }
        }
        printf("final while");
        free(filepath);
        strcat(buffer, "</tr>");
        // printf("buffer: %s\n",buffer);
    }
    strcat(buffer,"\0");
    printf("buffer: %s\n", buffer);
    return buffer;
}

char *HTTP_header(char *typecode, char *shortmsg)
{
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    sprintf(buffer,"HTTP/1.0 %s %s\r\n", typecode, shortmsg);
    strcat(buffer, "Server: webserver-c\r\n");
    strcat(buffer, "Content-type: text/html\r\n\r\n");
    return buffer;
}

char *createHTML(char *path)
{
    char *header = HTTP_header("200", "OK");
    // printf("Cabecera:%s\n",header);
    char *table = generateTable(path);
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    sprintf(buffer, header);
    strcat(buffer, "<html><head></head><body>");
    strcat(buffer, table);
    strcat(buffer, "</body></html>");
    return buffer;
}

int sendResponse(int sockfd, char *resp)
{
    int valwrite = write(sockfd, resp, strlen(resp));
    if (valwrite == -1)
    {
        perror("webserver (write)");
    }
    return valwrite;
}
