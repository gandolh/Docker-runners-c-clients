#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // for string manipulation
#include <netinet/in.h> // inet stuff
#include <sys/socket.h> // for sockets
#include <arpa/inet.h>  // for inet_addr method
#include <unistd.h>     // for read, write, close methods
#include <sys/un.h>     // for unix

#define SOCKET_PATH "/tmp/my_socket"
#define CLIENT_BUFFER_SIZE 4096
#define SERVER_BUFFER_SIZE 4096

int main()
{
    int sockfd;
    struct sockaddr_un serv_addr;
    int n;
    char request[CLIENT_BUFFER_SIZE];
    char response[SERVER_BUFFER_SIZE];

    // creating UDP socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("SOCKET OPENING ERROR");
        exit(EXIT_FAILURE);
    }

    // init adress stuff
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, SOCKET_PATH);

    int con_result = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (con_result == -1)
    {
        perror("Error while connecting");
        exit(1);
    }

    while (1)
    {
        printf(">>> ");
        // clear buffers with memset
        memset(request, 0, sizeof(request));
        memset(response, 0, sizeof(response));
        fgets(request, CLIENT_BUFFER_SIZE, stdin);

        // send command to server
        n = send(sockfd, request, CLIENT_BUFFER_SIZE, 0);
        if (n < 0)
        {
            perror("WRITING ERROR");
            exit(EXIT_FAILURE);
        }

        // check if user sent exit
        if (strcmp(request, "exit\n") == 0)
        {
            printf("Exiting...\n");
            break;
        }

        // get reply from server
        n = recv(sockfd, response, SERVER_BUFFER_SIZE, 0);

        if (n < 0)
        {
            perror("READING ERROR");
            exit(EXIT_FAILURE);
        }

        printf("Server: %s\n", response);
    }

    close(sockfd);
    unlink(SOCKET_PATH);
    exit(EXIT_SUCCESS);
}
