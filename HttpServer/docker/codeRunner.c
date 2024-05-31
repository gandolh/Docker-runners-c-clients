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
#include "uuid4.h"
#include <fcntl.h>
#define PORT 8080
#define MAX_QUEUE 5
#define REQ_BUFFER_SIZE 1024
#define RES_BUFFER_SIZE 1024
#define FILE_NAME_SIZE 50

void write_log(const char *format, ...)
{
    FILE *file = fopen("logs.txt", "a");
    if (file == NULL)
    {
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

void create_log_file()
{
    FILE *file = fopen("logs.txt", "w");
    if (file == NULL)
    {
        write_log("Error opening file!\n");
        return;
    }

    fclose(file);
    write_log("Log file created");
}

char *generate_json_resp(const char *status, const char *filename, const char *stdout, const char *stderr)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", status);
    cJSON_AddStringToObject(root, "filename", filename);
    cJSON_AddStringToObject(root, "stdout", stdout);
    cJSON_AddStringToObject(root, "stderr", stderr);

    char *json_string = cJSON_Print(root);

    cJSON_Delete(root);

    return json_string;
}

char *writeAndCompile(const char *code, const char *compiler, const char *extension)
{
    mkdir("code", 0777);
    mkdir("errors", 0777);
    mkdir("submissions", 0777);

    // Get current time
    char uuid4buf[UUID4_LEN];
    uuid4_generate(uuid4buf);

    // Create filename
    char filename[50];
    snprintf(filename, sizeof(filename), "code/%s.%s", uuid4buf, extension);
    // Create the error file path
    char err_file_path[256];
    sprintf(err_file_path, "errors/%s_error.txt", uuid4buf);
    // Create executable filename
    char exec_filename[50];
    snprintf(exec_filename, sizeof(exec_filename), "submissions/%s", uuid4buf);

    // Open file
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    if (fwrite(code, sizeof(char), strlen(code), file) != strlen(code))
    {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    write_log("File created: %s\n", filename);

    // Fork a new process
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        int err_file = open(err_file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (err_file == -1)
        {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Redirect stderr to the file
        if (dup2(err_file, STDERR_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        // Execute the compiler
        execlp(compiler, compiler, filename, "-o", exec_filename, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            write_log("Compilation successful\n");
            return generate_json_resp("success", exec_filename, "", "");
        }
        else
        {
            write_log("Compilation failed\n");

            // Read the error message from the file
            FILE *err_file = fopen(err_file_path, "r");
            if (err_file == NULL)
            {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            char *error_msg = malloc(256);   // allocate memory for the error message
            fgets(error_msg, 256, err_file); // read the error message
            fclose(err_file);

            return generate_json_resp("error", "", "", error_msg);
        }
    }
}

char *RunCompiled(const char *exec_path)
{
    int stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    // Fork a new process
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        close(stdout_pipe[0]); // Close the read end of the stdout pipe in the child
        close(stderr_pipe[0]); // Close the read end of the stderr pipe in the child

        dup2(stdout_pipe[1], STDOUT_FILENO); // Redirect stdout to the stdout pipe
        dup2(stderr_pipe[1], STDERR_FILENO); // Redirect stderr to the stderr pipe

        close(stdout_pipe[1]); // Close the write end of the stdout pipe in the child
        close(stderr_pipe[1]); // Close the write end of the stderr pipe in the child

        // Execute the compiled program
        execlp("/bin/sh", "/bin/sh", "-c", exec_path, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        close(stdout_pipe[1]); // Close the write end of the stdout pipe in the parent
        close(stderr_pipe[1]); // Close the write end of the stderr pipe in the parent

        char stdout_buffer[4096], stderr_buffer[4096];
        // clear buffers
        memset(stdout_buffer, 0, sizeof(stdout_buffer));
        memset(stderr_buffer, 0, sizeof(stderr_buffer));
        read(stdout_pipe[0], stdout_buffer, sizeof(stdout_buffer));
        read(stderr_pipe[0], stderr_buffer, sizeof(stderr_buffer));

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            int exit_status = WEXITSTATUS(status);
            // Generate a JSON response
            char *json_resp;
            if (strlen(stderr_buffer) > 0)
                json_resp = generate_json_resp("error", "", stdout_buffer, stderr_buffer);
            else
                json_resp = generate_json_resp("success", "", stdout_buffer, stderr_buffer);
            return json_resp;
        }
        else
        {
            char *json_resp = generate_json_resp("error", "", stdout_buffer, stderr_buffer);
            return json_resp;
        }
    }
}

char *WriteSourceCode(const char *code)
{
    mkdir("submissions", 0777);

    char buf[UUID4_LEN];
    uuid4_init();
    uuid4_generate(buf);

    char *filename = malloc(FILE_NAME_SIZE);
    snprintf(filename, FILE_NAME_SIZE, "submissions/%s", buf);

    // Open file
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    if (fwrite(code, sizeof(char), strlen(code), file) != strlen(code))
    {
        perror("fwrite");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    write_log("File created: %s\n", filename);
    return filename;
}

char *RunScript(const char *interpreter, const char *exec_path)
{
    int stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    // Fork a new process
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        close(stdout_pipe[0]); // Close the read end of the stdout pipe in the child
        close(stderr_pipe[0]); // Close the read end of the stderr pipe in the child

        dup2(stdout_pipe[1], STDOUT_FILENO); // Redirect stdout to the stdout pipe
        dup2(stderr_pipe[1], STDERR_FILENO); // Redirect stderr to the stderr pipe

        close(stdout_pipe[1]); // Close the write end of the stdout pipe in the child
        close(stderr_pipe[1]); // Close the write end of the stderr pipe in the child

        // Execute the compiled program
        execlp(interpreter, interpreter, exec_path, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        close(stdout_pipe[1]); // Close the write end of the stdout pipe in the parent
        close(stderr_pipe[1]); // Close the write end of the stderr pipe in the parent

        char stdout_buffer[4096], stderr_buffer[4096];
        memset(stdout_buffer, 0, sizeof(stdout_buffer));
        memset(stderr_buffer, 0, sizeof(stderr_buffer));

        read(stdout_pipe[0], stdout_buffer, sizeof(stdout_buffer));
        read(stderr_pipe[0], stderr_buffer, sizeof(stderr_buffer));

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            int exit_status = WEXITSTATUS(status);
            // Generate a JSON response
            char *json_resp;
            if (strlen(stderr_buffer) > 0)
                json_resp = generate_json_resp("error", "", stdout_buffer, stderr_buffer);
            else
                json_resp = generate_json_resp("success", "", stdout_buffer, stderr_buffer);
            return json_resp;
        }
        else
        {
            char *json_resp = generate_json_resp("error", "", stdout_buffer, stderr_buffer);
            return json_resp;
        }
    }
}

void handleMessage(char *request, char *buffer)
{
    // true if no operation was made
    int no_op = 1;
    cJSON *req_json = cJSON_ParseWithLength(request, REQ_BUFFER_SIZE);
    if (req_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        goto end;
    }

    const cJSON *type_json = NULL;
    type_json = cJSON_GetObjectItemCaseSensitive(req_json, "type");
    if (!cJSON_IsString(type_json) || (type_json->valuestring == NULL))
    {
        write_log("type not found in request\n");
    }
    const char *type = type_json->valuestring;
    if (strcmp(type, "COMPILE") == 0)
    {
        // do compile stuff
        const cJSON *code_json = NULL;
        const cJSON *language_json = NULL;
        const char *code = NULL;
        const char *language = NULL;

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

        if (strcmp(language, "C") == 0)
        {
            no_op = 0;
            char *respJson = writeAndCompile(code, "gcc", "c");
            strcpy(buffer, respJson);
            free(respJson);
        }
        else if (strcmp(language, "RUST") == 0)
        {
            no_op = 0;
            char *respJson = writeAndCompile(code, "rustc", "rs");
            strcpy(buffer, respJson);
            free(respJson);
        }
    }
    else if (strcmp(type, "RUN") == 0)
    {

        const cJSON *language_json = NULL;
        const char *language = NULL;
        const cJSON *filename_json = NULL;
        const char *filename = NULL;
        const cJSON *code_json = NULL;
        const char *code = NULL;

        language_json = cJSON_GetObjectItemCaseSensitive(req_json, "language");
        filename_json = cJSON_GetObjectItemCaseSensitive(req_json, "filename");
        code_json = cJSON_GetObjectItemCaseSensitive(req_json, "code");
        language = language_json->valuestring;
        filename = filename_json->valuestring;
        code = code_json->valuestring;
        if (!cJSON_IsString(language_json) || (language_json->valuestring == NULL))
        {
            write_log("language not found in request\n");
        }
        if (!cJSON_IsString(filename_json) || (filename_json->valuestring == NULL))
        {
            write_log("filename not found in request\n");
        }
        if (!cJSON_IsString(code_json) || (code_json->valuestring == NULL))
        {
            write_log("code not found in request\n");
        }

        // do run stuff
        if (strcmp(language, "C") == 0)
        {
            no_op = 0;
            char *respJson = RunCompiled(filename);
            sprintf(buffer, "%s", respJson);
        }
        else if (strcmp(language, "RUST") == 0)
        {
            no_op = 0;
            char *respJson = RunCompiled(filename);
            sprintf(buffer, "%s", respJson);
        }
        else if (strcmp(language, "PYTHON") == 0)
        {
            no_op = 0;
            char *filename = WriteSourceCode(code);
            char *json = RunScript("/usr/bin/python", filename);
            sprintf(buffer, "%s", json);
        }
    }
    if (no_op == 1)
    {
        write_log("Invalid request type\n");
        sprintf(buffer, "%s", "\"Invalid request type\"");
    }
end:
    cJSON_Delete(req_json);
}

int main()
{
    create_log_file();
    uuid4_init();
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

        memset(response, 0, RES_BUFFER_SIZE);
        memset(receivedMsg, 0, REQ_BUFFER_SIZE);
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