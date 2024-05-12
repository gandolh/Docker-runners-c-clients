#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include "cJSON.h"
#define PORT 8080
#define MAX_QUEUE 5
#define REQ_BUFFER_SIZE 1024
#define RES_BUFFER_SIZE 1024


void write_log(const char *format, ...) {
    FILE *file = fopen("logs.txt", "a");
    if (file == NULL) {
        printf("Error opening file!\n");
        return;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Format time, "ddd yyyy-mm-dd hh:mm:ss"
    char buf[80];
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", t);

    fprintf(file, "%s - ", buf);

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fprintf(file, "\n");
    fclose(file);
}

void create_log_file() {
    FILE *file = fopen("logs.txt", "w");
    if (file == NULL) {
        write_log("Error opening file!\n");
        return;
    }

    fclose(file);
    write_log("Log file created");
}

int writeAndCompileC(const char* code){
    mkdir("code", 0777);
    time_t now = time(NULL);
    char filename[50];
    snprintf(filename, sizeof(filename), "code/%ld.c", now);
    FILE *fd = fopen(filename, "w");
        if (fd == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

   if (fwrite(code, sizeof(char), strlen(code), fd) != strlen(code)) {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }

    fclose(fd);
    sleep(1);
    write_log("File created: %s\n", filename);
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close(pipefd[0]);  // Close the read end of the pipe in the child
        dup2(pipefd[1], STDOUT_FILENO);  // Redirect stdout to the pipe
        dup2(pipefd[1], STDERR_FILENO);  // Redirect stderr to the pipe
        close(pipefd[1]);  // Close the write end of the pipe in the child


        execlp("gcc", "gcc", filename, "-o", "code/tmp", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }else{
        close(pipefd[1]);  // Close the write end of the pipe in the parent
        char buffer[5120];
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[count] = '\0';
            write_log("%s", buffer);
        }
        close(pipefd[0]);  // Close the read end of the pipe in the parent


         int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            write_log("Compilation successful\n");
            return 0;
        } else {
            write_log("Compilation failed\n");
            return 1;
        }
    }
}

void handleMessage(char *request, char *buffer)
{
    // Split the request into method and params
    char *method = strtok(request, ":");
    char *params = strtok(NULL, "");   
    cJSON *req_json = cJSON_ParseWithLength(params, REQ_BUFFER_SIZE);
    const cJSON *code_json = NULL;
    const cJSON *language_json = NULL;
    const char* code = NULL;
    const char* language = NULL;

    if (req_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        goto end;
    }

    code_json = cJSON_GetObjectItemCaseSensitive(req_json, "code");
    language_json = cJSON_GetObjectItemCaseSensitive(req_json, "language");
    code = code_json->valuestring;
    language = language_json->valuestring;
    
    if (!cJSON_IsString(code_json) || (code_json->valuestring == NULL))
    {
        write_log("code not found in request\n");
    }
    if (!cJSON_IsString(language_json) || (language_json->valuestring == NULL))
    {
        write_log("language not found in request\n");
    }

    // Check the method and set the buffer accordingly
    if (strncmp(method, "COMPILE_C_CODE", 14) == 0)
    {
        int ok =  writeAndCompileC(code);
        sprintf(buffer, "%s", ok == 0 ? "Compilation successful" : "Compilation failed");
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
        write_log("Invalid request type\n");
    }
    end:
    cJSON_Delete(req_json);
}



int main()
{
    create_log_file();
    write_log("Starting Code Runner\n");
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
    write_log("socket created\n");


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
    write_log("host bind done\n");
    write_log("Start listening for the clients");
    listen(sockfd, MAX_QUEUE);
    write_log("Server listening on port %d\n", PORT);

    clilen = sizeof(cli_addr);
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
        write_log("Received message: %s\n", receivedMsg);
        handleMessage(receivedMsg, response);
        write_log("Sending response: %s\n", response);
        write(newsockfd, response, strlen(response));
        close(newsockfd);
    }

    close(sockfd);
    return 0;
}