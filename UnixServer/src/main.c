#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>   // for wait method
#include <string.h>     // for string manipulation
#include <netinet/in.h> // inet stuff
#include <sys/socket.h> // for sockets
#include <arpa/inet.h>  // for inet_addr method
#include <unistd.h>     // for read, write, close methods
#include <sys/un.h>     // for unix
#include <time.h>
#include "Database.h"

#define SOCKET_PATH "/tmp/my_socket"
#define CLIENT_BUFFER_SIZE 9000
#define SERVER_BUFFER_SIZE 9000
#define CMD_MAX_SIZE 9000

time_t start_time;
static char *help_text = "\nList of commands:\n"
                         "help - shows list of commands\nuptime - shows server uptime"
                         "\ncmd [your linux command] - executes given command and prints"
                         "results\nall clients - returns a list of all clients\n"
                         "connected clients - returns a list of conected clients\n"
                         "submissions - show submissions";

char *executeCommand(char *command, char *argument)
{
    pid_t pid;
    int pipefd[2];
    char *output = NULL;
    size_t output_size = 0;

    // remove trailing \n
    command[strcspn(command, "\n")] = 0;

    // create pipe
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // close read end of pipe
        close(pipefd[0]);

        // redirect stdout to the write end of the pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // close write end of pipe
        close(pipefd[1]);

        // execute the command
        if (execlp(command, command, argument, NULL) == -1)
        {
            perror("execlp");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // close the write end of the pipe
        close(pipefd[1]);

        char buffer[CMD_MAX_SIZE];
        ssize_t bytes_read;

        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0)
        {
            output = realloc(output, output_size + bytes_read + 1);
            if (output == NULL)
            {
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

char *getUptime()
{
    time_t current_time;
    time(&current_time);
    double diff = difftime(current_time, start_time);

    int days = (int)(diff / (60 * 60 * 24));
    diff -= days * (60 * 60 * 24);
    int hours = (int)(diff / (60 * 60));
    diff -= hours * (60 * 60);
    int minutes = (int)(diff / 60);
    diff -= minutes * 60;
    int seconds = (int)diff;

    static char uptime_str[20];
    sprintf(uptime_str, "%d %02d:%02d:%02d", days, hours, minutes, seconds);
    return uptime_str;
}

void handleMessage(char *request, int *client_socket)
{
    char response[SERVER_BUFFER_SIZE];
    memset(response, 0, SERVER_BUFFER_SIZE);

    if (strncmp(request, "cmd ", 4) == 0)
    {
        // extract inputed command
        char cmd[CLIENT_BUFFER_SIZE];
        strcpy(cmd, request + 4);
        char *output = executeCommand(cmd, NULL);
        sprintf(response, "%s", output);
    }
    else if (strcmp(request, "uptime\n") == 0)
    {
        char *uptime = getUptime();
        sprintf(response, "%s", uptime);
    }
    else if (strcmp(request, "all clients\n") == 0)
    {
        char *users = gettotalusers(10, SERVER_BUFFER_SIZE);
        sprintf(response, "\n%s", users);
    }
    else if (strcmp(request, "connected clients\n") == 0)
    {
        char *users = getconnectedusers(10, SERVER_BUFFER_SIZE);
        sprintf(response, "\n%s", users);
    }
    else if (strcmp(request, "submissions\n") == 0)
    {
        char *submissions = getSubmissions(10, SERVER_BUFFER_SIZE);
        sprintf(response, "\n%s", submissions);
    }
    else if (strcmp(request, "help\n") == 0)
    {
        sprintf(response, "%s", help_text);
    }
    else if (strcmp(request, "exit\n") == 0)
    {
        printf("Exiting...\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        sprintf(response, "%s", "No such command");
    }
    send(*client_socket, response, strlen(response), 0);
}
int main()
{
    int sockfd, client_socket;
    socklen_t clilen;
    time(&start_time);

    int n;
    char request[CLIENT_BUFFER_SIZE];
    struct sockaddr_un serv_addr, cli_addr;

    // creating UDP socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    // init server stuff
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, SOCKET_PATH);

    // unlink before binding
    unlink(SOCKET_PATH);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(EXIT_FAILURE);
    }

    listen(sockfd, 1);
    printf("Server is listening...\n");

    clilen = sizeof(cli_addr);

    client_socket = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    while (1)
    {
        // empty request buffer and response buffer
        memset(request, 0, CLIENT_BUFFER_SIZE);
        ssize_t numBytes = recv(client_socket, request, CLIENT_BUFFER_SIZE, 0);
        if (numBytes < 0)
        {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        else if (numBytes == 0)
        {
            printf("Client disconnected\n");
            break;
        }
        else
        {
            handleMessage(request, &client_socket);
        }
    }

    close(sockfd);
    unlink(SOCKET_PATH);
    exit(EXIT_SUCCESS);
}