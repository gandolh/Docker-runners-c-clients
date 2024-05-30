/**
 * Examples of files to run:
 * - C: examples/example.c
 * - Rust: examples/example.rs
 * - Python: examples/example.py
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cJSON.h"
#include "Logger.h"
#include <unistd.h>

#define PORT 6969
#define CLIENT_BUFFER_SIZE 4096
#define RESPONSE_DATA_BUFFER_SIZE 4096
#define HEADER_BUFFER_SIZE 20
#define RESPONSE_BUFFER_SIZE RESPONSE_DATA_BUFFER_SIZE + HEADER_BUFFER_SIZE

typedef struct ServerRequest
{
    char *language;
    char *code;
} ServerRequest;

void initializeRequest(ServerRequest *request, const char *language, const char *code)
{
    request->language = strdup(language);
    request->code = strdup(code);
}

void freeRequest(ServerRequest *request)
{
    free(request->language);
    free(request->code);
}

void promptForInput(char *buffer, size_t size, const char *prompt)
{
    printf("%s", prompt);
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
}

const char *getLanguageFromInput(int input)
{
    switch (input)
    {
    case 1:
        return "PYTHON";
    case 2:
        return "RUST";
    case 3:
        return "C";
    default:
        return NULL;
    }
}

// return 0 if success, -1 if failed
int handleServerLogin(int *sockfd)
{

    char username[256];
    char password[256];

    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // Remove trailing newline

    printf("Enter password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;

    cJSON *login_json = cJSON_CreateObject();
    cJSON_AddStringToObject(login_json, "username", username);
    cJSON_AddStringToObject(login_json, "password", password);

    char *login_json_string = cJSON_Print(login_json);
    char *http_login_request = malloc(strlen("POST /api/login HTTP/1.1\r\n"
                                             "Host: localhost\r\n"
                                             "Content-Type: application/json\r\n"
                                             "Content-Length: ") +
                                      strlen(login_json_string) +
                                      strlen("\r\n\r\n") +
                                      strlen(login_json_string) +
                                      1);
    sprintf(http_login_request, "POST /api/login HTTP/1.1\r\n"
                                "Host: localhost\r\n"
                                "Content-Type: application/json\r\n"
                                "Content-Length: %zu\r\n\r\n"
                                "%s",
            strlen(login_json_string), login_json_string);

    // Send the HTTP request for login
    ssize_t sent_login_size = send(*sockfd, http_login_request, CLIENT_BUFFER_SIZE, 0);
    if (sent_login_size < 0)
    {
        write_log("ERROR: send failed");
        return -1;
    }

    char response[CLIENT_BUFFER_SIZE];
    memset(response, 0, sizeof(response));

    ssize_t received_size = recv(*sockfd, response, CLIENT_BUFFER_SIZE - 1, 0);
    if (received_size < 0)
    {
        write_log("ERROR: recv failed");
        return -1;
    }

    char *header_end = strstr(response, "\r\n\r\n");
    if (header_end == NULL)
    {
        write_log("ERROR: Response does not contain a valid HTTP header");
        return -1;
    }

    char *body = header_end + 4; // Skip past the "\r\n\r\n"

    cJSON *json_response = cJSON_Parse(body);
    if (json_response == NULL)
    {
        write_log("ERROR: Failed to parse JSON response");
        return -1;
    }

    cJSON *status_json = cJSON_GetObjectItemCaseSensitive(json_response, "status");
    if (cJSON_IsString(status_json) && status_json->valuestring != NULL)
    {
        if (strcmp(status_json->valuestring, "success") == 0)
        {
            printf("Login successful\n");
        }
        else if (strcmp(status_json->valuestring, "failed") == 0)
        {
            printf("Login failed\n");
            exit(-1);
        }
        else
        {
            printf("Unknown status: %s\n", status_json->valuestring);
        }
    }

    cJSON_Delete(json_response);
    free(login_json_string);
    free(http_login_request);
}

int main()
{
    char inputBuffer[10];
    char code[256];
    char fileOption[10];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Cannot create socket");
        return 1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Cannot connect to server");
        return 1;
    }

    handleServerLogin(&sockfd);

    while (1)
    {
        // empty inputBuffer, code and fileOption
        memset(inputBuffer, 0, sizeof(inputBuffer));
        memset(code, 0, sizeof(code));
        memset(fileOption, 0, sizeof(fileOption));
        promptForInput(inputBuffer, sizeof(inputBuffer), "Enter language (1 for Python, 2 for Rust, 3 for C, or '0' to quit): ");
        int languageOption = atoi(inputBuffer);
        if (languageOption == 0)
        {
            break;
        }

        const char *language = getLanguageFromInput(languageOption);
        if (language == NULL)
        {
            printf("Invalid option. Please try again.\n");
            continue;
        }

        promptForInput(code, sizeof(code), "Enter code or filename: ");
        promptForInput(fileOption, sizeof(fileOption), "Is it a file? (yes/no): ");

        int isFile = strcmp(fileOption, "yes") == 0;

        ServerRequest request;
        if (isFile)
        {
            FILE *file = fopen(code, "r");
            if (!file)
            {
                perror("Failed to open file");
                continue;
            }
            fseek(file, 0, SEEK_END);
            long length = ftell(file);
            fseek(file, 0, SEEK_SET);
            char *buffer = malloc(length + 1);
            if (buffer)
            {
                fread(buffer, 1, length, file);
                buffer[length] = '\0';
            }
            fclose(file);
            initializeRequest(&request, language, buffer);
            free(buffer);
        }
        else
        {
            initializeRequest(&request, language, code);
        }

        // Here you can add the code to actually send the request to the server
        write_log("Sending request to server");

        // Convert ServerRequest to cJSON
        cJSON *request_json = cJSON_CreateObject();
        cJSON_AddStringToObject(request_json, "language", request.language);
        cJSON_AddStringToObject(request_json, "code", request.code);

        // Print the JSON
        char *json_string = cJSON_Print(request_json);

        // Create the HTTP request
        char *http_request = malloc(strlen("POST /api/run HTTP/1.1\r\n"
                                           "Host: localhost\r\n"
                                           "Content-Type: application/json\r\n"
                                           "Content-Length: ") +
                                    strlen(json_string) +
                                    strlen("\r\n\r\n") +
                                    strlen(json_string) +
                                    1);
        sprintf(http_request, "POST /api/run HTTP/1.1\r\n"
                              "Host: localhost\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: %zu\r\n\r\n"
                              "%s",
                strlen(json_string), json_string);

        // Send the HTTP request
        ssize_t sent_size = send(sockfd, http_request, CLIENT_BUFFER_SIZE, 0);
        if (sent_size < 0)
        {
            write_log("ERROR: send failed");
            return -1;
        }

        char buffer[RESPONSE_BUFFER_SIZE];
        memset(buffer, 0, RESPONSE_BUFFER_SIZE);

        ssize_t recv_size = recv(sockfd, buffer, RESPONSE_BUFFER_SIZE, 0);
        if (recv_size < 0)
        {
            perror("ERROR: recv failed");
            write_log("ERROR: recv failed");
            return -1;
        }
        else if (recv_size == 0)
        {
            write_log("Server closed the connection\n");
            return 0;
        }

        // Write the response to the logs
        write_log("Response from server: %s", buffer);

        // Free the cJSON object and the strings
        cJSON_Delete(request_json);
        free(json_string);
        free(http_request);
        freeRequest(&request);
    }

    return 0;
}