#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h> // for wait method
#include <string.h> // for string manipulation
#include <netinet/in.h> // inet stuff
#include <sys/socket.h> // for sockets
#include <arpa/inet.h> // for inet_addr method
#include <unistd.h> // for read, write, close methods
#include <sys/un.h>// for unix

#define SOCKET_PATH "/tmp/my_socket"
#define MAX 1024

char* executeCommand(char *command, char* argument) {
    pid_t pid;
    int pipefd[2];
    char *output = NULL;
    size_t output_size = 0;

    // remove trailing \n
    command[strcspn(command, "\n")] = 0;

    // create pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // close read end of pipe
        close(pipefd[0]);

        // redirect stdout to the write end of the pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // close write end of pipe
        close(pipefd[1]);

        // execute the command
        if (execlp(command, command, argument, NULL) == -1) {
            perror("execlp");
            exit(EXIT_FAILURE);
        }
    } else {
        // close the write end of the pipe
        close(pipefd[1]);

        char buffer[MAX]; // buffer for reading from pipe
        ssize_t bytes_read;

        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            output = realloc(output, output_size + bytes_read + 1);
            if (output == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }

            memcpy(output + output_size, buffer, bytes_read);
            output_size += bytes_read;

            output[output_size] = '\0';
        }

        close(pipefd[0]);
        wait(NULL);
    }

    return output;
}

char* getStats() {
    FILE* fp;
    char buffer[MAX]; 
    char* result = NULL;
    size_t result_size = 0;
    size_t line_length;

    fp = popen("top -bn1", "r");
    if (fp == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL){
        line_length = strlen(buffer);

        result = realloc(result, result_size + line_length + 1);
        if (result == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }

        strcpy(result + result_size, buffer);
        result_size += line_length;
    }

    if (pclose(fp) == -1) {
        perror("pclose");
        exit(EXIT_FAILURE);
    }

    return result;
}

int main() {
    int sockfd, client_socket;
    socklen_t clilen;
    
    int n;
    char buffer[MAX];
    struct sockaddr_un serv_addr, cli_addr;

    char * help_text = "\nList of commands:\nhelp - shows list of commands\nuptime - shows server uptime\ncmd [your linux command] - executes given command and prints results\ntotal clients - returns number of total clients\n";

    // creating UDP socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    // init server stuff
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, SOCKET_PATH);

    // unlink before binding
    unlink(SOCKET_PATH);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("ERROR on binding");
        exit(EXIT_FAILURE);
    }

    listen(sockfd, 1);
    printf("Server is listening...\n");

    clilen = sizeof(cli_addr);

    client_socket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    while (1) {

        // get command from client
        bzero(buffer, sizeof(buffer));
        n = recv(client_socket, buffer, sizeof(buffer), 0);
        if (n < 0){
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        printf("\nUser said: %s", buffer);

        if(strncmp(buffer, "cmd ", 4) == 0){
            // extract inputed command
            char cmd[MAX];
            strcpy(cmd, buffer + 4);
            
            char *output = executeCommand(cmd, NULL);
            send(client_socket, output, strlen(output), 0);
        }
        else if (strcmp(buffer, "uptime\n") == 0) {
            char cmd[MAX] = "w\n";

            char *uptime = executeCommand(cmd, NULL);
            send(client_socket, uptime, strlen(uptime), 0);
        }
        else if (strcmp(buffer, "total clients\n") == 0) {
            // TODO: add logic to fetch user count
            send(client_socket, "245", strlen("245"), 0);
        }
        else if (strcmp(buffer, "help\n") == 0) {
            send(client_socket, help_text, strlen(help_text), 0);
        }
        else if (strcmp(buffer, "exit\n") == 0) {
            printf("Exiting...\n");
            break;
        }
        else {
            char *unknown_command = "No such command";
            send(client_socket, unknown_command, strlen(unknown_command), 0);
        }
    }

    close(sockfd);
    unlink(SOCKET_PATH);
    exit(EXIT_SUCCESS);
}