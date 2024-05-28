#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for string manipulation
#include <netinet/in.h> // inet stuff
#include <sys/socket.h> // for sockets
#include <arpa/inet.h> // for inet_addr method
#include <unistd.h> // for read, write, close methods
#include <sys/un.h>// for unix

#define SOCKET_PATH "/tmp/my_socket"
#define MAX 1024

int main() {
    int sockfd;
    struct sockaddr_un serv_addr;
    int n;
    char buffer[256];

    // creating UDP socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("SOCKET OPENING ERROR");
        exit(EXIT_FAILURE);
    }

    // init adress stuff
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, SOCKET_PATH);

    int con_result = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (con_result == -1) {
        perror("Error while connecting");
        exit(1);
    }

    while (1) {
        printf(">>> ");
        bzero(buffer, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);

        // send command to server
        n = send(sockfd, buffer, strlen(buffer), 0);
        if (n < 0){
            perror("WRITING ERROR");
            exit(EXIT_FAILURE);
        }

        // check if user sent exit
        if (strcmp(buffer, "exit\n") == 0) {
            printf("Exiting...\n");
            break;
        }

        // get reply from server
        bzero(buffer, sizeof(buffer));
        n = recv(sockfd, buffer, sizeof(buffer), 0);

        if (n < 0){
            perror("READING ERROR");
            exit(EXIT_FAILURE);
        }

        printf("Server: %s\n", buffer);
    }

    close(sockfd);
    unlink(SOCKET_PATH);
    exit(EXIT_SUCCESS);
}
