#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_QUEUE 5
#define REQ_BUFFER_SIZE 1024
#define RES_BUFFER_SIZE 1024

void handleMessage(char *request, char *buffer)
{
    // Split the request into method and params
    char *method = strtok(request, ":");
    char *params = strtok(NULL, ":");

    // Check the method and set the buffer accordingly
    if (strncmp(method, "COMPILE_C_CODE", 14) == 0)
    {
        sprintf(buffer, "refId:%s", params);
    }
    else if (strncmp(method, "COMPILE_RUST_CODE", 17) == 0)
    {
        sprintf(buffer, "rustc %s", params);
    }
    else if (strncmp(method, "RUN_COMPILED_C_CODE", 19) == 0)
    {
        sprintf(buffer, "./%s", params);
    }
    else if (strncmp(method, "RUN_COMPILED_RUST_CODE", 22) == 0)
    {
        sprintf(buffer, "./%s", params);
    }
    else if (strncmp(method, "RUN_PYTHON_CODE", 15) == 0)
    {
        sprintf(buffer, "python3 %s", params);
    }
    else
    {
        printf("Invalid request type\n");
    }
}

int main()
{
    int sockfd, newsockfd;
    socklen_t clilen;
    char response[RES_BUFFER_SIZE];
    char receivedMsg[REQ_BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }


    // Initialize socket structure
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind the host address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    // Start listening for the clients
    listen(sockfd, MAX_QUEUE);

    clilen = sizeof(cli_addr);
    printf("Server listening on port %d\n", PORT);
    while (1)
    {

        memset(response, 0, sizeof(response));
        memset(receivedMsg, 0, sizeof(receivedMsg));
        // Accept actual connection from the client
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }
        ssize_t n = read(newsockfd, receivedMsg, sizeof(receivedMsg) - 1);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            exit(1);
        }
        printf("Received message: %s\n", receivedMsg);
        handleMessage(receivedMsg, response);
        printf("Sending response: %s\n", response);
        write(newsockfd, response, strlen(response));
        close(newsockfd);
    }

    close(sockfd);
    return 0;
}