#include "../inc/web-sever.h"
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <fcntl.h>
#include <libgen.h>
#define PATH_TEMPLATE "../Template/filesDirectory.html"
// #include "../inc/URI_parser.h"
#define BUFFER_TABLE 1048576
#ifndef URI_PARSER_H
#define URI_PARSER_H

char *Uri_parser(char *uri);
char HexaToDec(char *Hex);

#endif
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
    signal(SIGCHLD, SignHandlerKillChild);
#pragma region Variables
    struct sockaddr_in host_addr;
    int host_addrlen = sizeof(host_addr);

    memset(&host_addr, 0, sizeof(host_addr));
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

        int childpid = childProcess(newsockfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen, path);
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
int childProcess(int client_fd, struct sockaddr *client_addr, socklen_t *client_addrlen, char *path)
{
    pid_t childPid = fork();
    if (childPid == 0)
    {
        struct httpRequest *request = readRequest(client_fd);
        char *response = processResponse(request, client_fd, path);
        sendResponse(client_fd, response);
        free(response);
        free(request->button);
        free(request->method);
        free(request->uri);
        free(request->version);
        free(request);
    }
    return childPid;
    // return 1;
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

    request->method = (char *)malloc(BUFFER_SIZE);
    request->uri = (char *)malloc(BUFFER_SIZE);
    request->version = (char *)malloc(BUFFER_SIZE);
    request->button = (char *)malloc(BUFFER_SIZE);
    sscanf(buffer, "%s %s %s", request->method, request->uri, request->version);

    if (strcmp(request->method, "POST") == 0)
    {
        strcpy(request->button, readAction(buffer));
    }
    return request;
}

char *processResponse(struct httpRequest *request, int sockfd, char *path)
{
    // printf("%s\n", request->button);
    printf("----------------------------------------------------------------------------------------\n");
    printf("[Methods|Uri|Button|Version]:[%s|%s|%s|%s]\n", request->method, request->uri, request->button, request->version);
    printf("----------------------------------------------------------------------------------------\n");
    char *buffer;
    char *uri = Uri_parser(request->uri);
    // printf("%s\n", uri);
    // printf("%s\n", request->button);
    if (strcmp(request->method, "GET") == 0)
    {
        if (strcmp(uri, "/") == 0)
        {
            uri = strcpy(uri, path);
            printf("%s\n", uri);
        }
        struct stat folder;
        if (stat(uri, &folder) == 0)
        {
            if (S_ISDIR(folder.st_mode))
            {

                buffer = generateHTML(uri, PATH_TEMPLATE);
            }
            else
            {
                Download(sockfd, uri, folder.st_size);
            }
        }
        else if (strstr(uri, path) != NULL)
        {
            // error 404
            buffer = response_error("404", "Not Found", path);
        }
    }

    else if (strcmp(request->method, "POST") == 0)
    {
        if (strcmp(request->button, "root") == 0)
        {
            printf("%s", path);
            buffer = generateHTML(path, PATH_TEMPLATE);
        }
        else
        {
            buffer = response_error("501", "Not Implemented", path);
        }
    }
    else
    {
        buffer = response_error("501", "Not Implemented", path);
    }
    return buffer;
}

char *generateTable(char *path)
{ // TODO: revisar algunas comprobaciones
    char *buffer = (char *)malloc(BUFFER_TABLE * sizeof(char));
    // printf("path:%s\n", path);
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
        if (strcmp(ent->d_name, ".") == 0)
            continue;
        if (strcmp(ent->d_name, "..") == 0)
            continue;

        // printf("filepath %s\n", filepath);

        struct stat fileInf;
        if (stat(filepath, &fileInf) < 0)
            continue;

        strcat(buffer, "<tr>");
        for (size_t j = 0; j < 3; j++)
        {
            // printf("Entto \" Forr %d\n", j);
            // printf("LLEGO\n");

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
        free(filepath);
        strcat(buffer, "</tr>");
        // printf("buffer: %s\n",buffer);
    }
    strcat(buffer, "\0");
    // printf("buffer: %s\n", buffer);
    return buffer;
}

char *HTTP_header(char *typecode, char *shortmsg)
{
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    sprintf(buffer, "HTTP/1.0 %s %s\r\n", typecode, shortmsg);
    strcat(buffer, "Server: webserver-c\r\n");
    strcat(buffer, "Content-type: text/html\r\n\r\n");
    return buffer;
}

int sendResponse(int sockfd, char *resp)
{
    char *header = HTTP_header("200", "OK");
    int valwrite = write(sockfd, header, strlen(resp));
    valwrite = write(sockfd, resp, strlen(resp));
    if (valwrite == -1)
    {
        perror("webserver (write)");
    }
    return valwrite;
}

char *Uri_parser(char *uri)
{
    char *ptr = uri;
    char *result = calloc(1, strlen(uri));
    char current_hex[3];
    while ((ptr = strchr(ptr, '%')) != NULL)
    {
        *(ptr++) = '\0';
        result = strcat(result, uri);
        uri = ptr + 2;
        strncpy(current_hex, ptr, 2);
        char c1 = HexaToDec(current_hex);
        result = strncat(result, &c1, 1);
    }
    strcat(result, uri);
    return result;
}
char HexaToDec(char *Hex)
{
    int dec = 0;
    int base = 1;
    int len = strlen(Hex);
    for (int i = len - 1; i >= 0; i--)
    {
        switch (Hex[i])
        {
        case 'A':
        case 'a':
            dec += 10 * base;
            break;
        case 'B':
        case 'b':
            dec += 11 * base;
            break;
        case 'C':
        case 'c':
            dec += 12 * base;
            break;
        case 'D':
        case 'd':
            dec += 13 * base;
            break;
        case 'E':
        case 'e':
            dec += 14 * base;
            break;
        case 'F':
        case 'f':
            dec += 15 * base;
            break;
        default:
            dec += ((int)Hex[i] - 48) * base;
            break;
        }

        base *= 16;
    }
    return (char)dec;
}

char *readAction(char *request)
{
    char *action = strstr(request, "root");
    if (action != NULL)
        return "root";
    else
        return NULL;
}
void SignHandlerKillChild(int sig)
{
    while (waitpid(-1, 0, WNOHANG) > 0)
        return;
}
char *parentDirectory(char *path)
{
    char *parent = strdup(path);
    if (parent == NULL)
    {
        perror("Error allocating memory");
        return NULL;
    }
    char *dir = dirname(parent);

    char *result = (char *)malloc(BUFFER_SIZE * sizeof(char));
    if (result == NULL)
    {
        perror("Error allocating memory");
        free(parent);
        return NULL;
    }
    strcpy(result, dir);
    free(parent);
    return result;
}
void Download(int fd, char *filename, int size)
{
    char *buff = calloc(1, 2048);
    char *buff2 = calloc(1, 2048);
    char *buff3 = calloc(1, 2048);

    sprintf(buff, "HTTP/1.1 200 OK\r\n");
    strcat(buff, "MIME-Version: 1.0\r\n");
    strcat(buff, "Content-Type: application/octet-stream\r\n");
    sprintf(buff2, "Content-Disposition: attacment;filename=\"%s\"\r\n", strrchr(filename, '/') + 1);
    strcat(buff, buff2);
    sprintf(buff3, "Content-Length: %d \r\n\r\n", size);
    strcat(buff, buff3);
    write(fd, buff, strlen(buff));

    free(buff);
    free(buff2);
    free(buff3);

    int filefd = open(filename, O_RDONLY, 0);
    off_t offset = 0;

    while (size > 0)
    {
        int curr_size = size;
        if (size > 2000000000)
            curr_size = 2000000000;

        if (sendfile(fd, filefd, &offset, curr_size) < 0)
        {
            if (errno == EINTR)
            {
                curr_size = 0;
            }
            else
            {
                printf("Error sending file");
                return;
            }
        }
        size -= curr_size;
    }

    write(fd, "\r\n", 2);
    close(filefd);
}

char *read_html_file(char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror("Error opening file\n");
        return NULL;
    }
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    if (buffer == NULL)
    {
        perror("Error allocating memory\n");
        fclose(fp);
        return NULL;
    }
    int byte_read = 0;
    int buffer_size = BUFFER_SIZE;
    while (1)
    {
        if (byte_read + BUFFER_SIZE > buffer_size)
        {
            buffer_size *= 2;
            buffer = (char *)realloc(buffer, buffer_size);
            if (buffer == NULL)
            {
                perror("Error reallocating memory\n");
                fclose(fp);
                return NULL;
            }
        }
        int valread = read(fileno(fp), buffer + byte_read, BUFFER_SIZE);
        if (valread < 0)
        {
            perror("webserver (read)\n");
        }
        byte_read += valread;
        if (valread == 0)
        {
            break;
        }
    }
    strcat(buffer, "\0");
    // printf("%s", buffer);
    fclose(fp);
    return buffer;
}

char *generateHTML(char *path, char *pathTemplate)
{
    char *buffer = malloc(BUFFER_SIZE * sizeof(char));
    buffer = read_html_file(pathTemplate);
    char *table = (char *)malloc(BUFFER_TABLE * sizeof(char));
    table = generateTable(path);
    char *html = (char *)malloc(BUFFER_TABLE * sizeof(char));
    sprintf(html, buffer);
    char *button_previous = (char *)malloc(BUFFER_SIZE * sizeof(char *));
    char *dirParent = parentDirectory(path);
    sprintf(button_previous, "<a href=\"");
    strcat(button_previous, dirParent);
    strcat(button_previous, "\" class=\"custom-button \">");
    strcat(button_previous, "Previous");
    strcat(button_previous, "</a>");
    strcat(button_previous, "</div>");
    strcat(html, button_previous);
    strcat(html, table);
    strcat(html,"<footer><p>&copy; 2023 Files Directory. All rights reserved.</p></footer></body></html>");
    return html;
}

char *response_error(char *typecode, char *shortmsg, char *path)
{
    char *buffer = (char *)malloc(5000);

    buffer = HTTP_header(typecode, shortmsg);

    if (strcmp(typecode, "501"))
    {
        return strcat(buffer, read_html_file("Template/Error501.html"));
    }

    if (strcmp(typecode, "404"))
    {
        return strcat(buffer, read_html_file("Template/Error404.html"));
    }

    char *link = (char *)malloc(300);

    sprintf(link, "<a href = \"%s\"></a>", path);
    strcat(link, "</div></body></html>) ");
    strcat(buffer, link);
    free(link);
    return buffer;
}
