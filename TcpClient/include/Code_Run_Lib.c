#include "Code_Run_Lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "Logger.h"
#include "cJSON.h"

#define MAX_ID_LENGTH 65
#define NUM_CONTAINERS 1
#define REQ_BUFFER_SIZE 1024
#define RES_BUFFER_SIZE 1024

int container_ports[NUM_CONTAINERS];

void InitContainersThreadpool()
{
    // Build the Docker image
    system("docker build -t multi_lang_container ./Docker");

    for (int i = 0; i < NUM_CONTAINERS; i++)
    {
        char container_name[20];
        sprintf(container_name, "CodeRunner_%d", i);

        // Command to remove existing container
        char remove_command[50];
        sprintf(remove_command, "docker rm -f %s", container_name);
        system(remove_command);

        // Command to run a new Docker container with the specified name
        int port = 8080 + i;
        char run_command[100];
        sprintf(run_command, "docker run -d --name %s -p %d:8080 multi_lang_container",
                container_name, port);

        system(run_command);

        // Save the port number
        container_ports[i] = port;
    }
    sleep(1);
}

void printContainersPorts()
{
    for (int i = 0; i < NUM_CONTAINERS; i++)
    {
        write_log("Container %d port: %d\n", i, container_ports[i]);
    }
}

void SendRequest(int port, char *msg, char *response)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    // Set server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("ERROR setting server address");
        exit(1);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting to the server");
        exit(1);
    }

    // Send the message
    if (write(sockfd, msg, REQ_BUFFER_SIZE) < 0)
    {
        perror("ERROR writing to the socket");
        exit(1);
    }

    // Read the response
    memset(response, 0, RES_BUFFER_SIZE);
    ssize_t n = read(sockfd, response, RES_BUFFER_SIZE - 1);
    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

    close(sockfd);
}

int getAvailablePort()
{
    // TODO: Implement a better algorithm to get the available port
    return container_ports[0];
}

REST_CompileResp *ToREST_CompileResp(char *response)
{
    REST_CompileResp *result = malloc(sizeof(REST_CompileResp));
    char *token = strtok(response, ",");
    while (token != NULL)
    {
        char propName[50];
        char propValue[50];
        sscanf(token, "%[^:]:%s", propName, propValue);
        if (strcmp(propName, "refId") == 0)
        {
            result->refId = malloc(50);
            strcpy(result->refId, propValue);
        }
        token = strtok(NULL, ",");
    }
    return result;
}

REST_CodeRunResp *ToREST_CodeRunResp(char *response)
{
    REST_CodeRunResp *result = malloc(sizeof(REST_CodeRunResp));
    char *token = strtok(response, ",");
    while (token != NULL)
    {
        char propName[50];
        char propValue[50];
        sscanf(token, "%[^:]:%s", propName, propValue);
        if (strcmp(propName, "output") == 0)
        {
            strcpy(result->output, propValue);
        }
        else if (strcmp(propName, "error") == 0)
        {
            strcpy(result->error, propValue);
        }
        token = strtok(NULL, ",");
    }
    return result;
}

REST_CompileResp *CompileCCode(char* json)
{
    char msg[REQ_BUFFER_SIZE] = "COMPILE_C_CODE:";
    strcat(msg, json);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), msg, response);
    write_log("Response: %s\n", response);
    // REST_CompileResp *result = ToREST_CompileResp(response);
    // free(response);
    // return result;
    return NULL;
}

REST_CompileResp *CompileRustCode(REST_CompileReq *req)
{
    char msg[REQ_BUFFER_SIZE] = "COMPILE_RUST_CODE:";
    char serialized_req[REQ_BUFFER_SIZE];
    sprintf(serialized_req, "language:%s,code:%s", req->language, req->code);
    strcat(msg, serialized_req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), msg, response);
    REST_CompileResp *result = ToREST_CompileResp(response);
    free(response);
    return result;
}

REST_CodeRunResp *RunCCode(REST_CodeRunReq *req)
{
    char msg[REQ_BUFFER_SIZE] = "RUN_COMPILED_C_CODE:";
    char serialized_req[REQ_BUFFER_SIZE];
    sprintf(serialized_req, "refId:%s,language:%s,code:%s", req->refId, req->language, req->code);
    strcat(msg, serialized_req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), msg, response);
    REST_CodeRunResp *result = ToREST_CodeRunResp(response);
    free(response);
    return result;
}

REST_CodeRunResp *RunRustCode(REST_CodeRunReq *req)
{
    char msg[REQ_BUFFER_SIZE] = "RUN_COMPILED_RUST_CODE:";
    char serialized_req[REQ_BUFFER_SIZE];
    sprintf(serialized_req, "refId:%s,language:%s,code:%s", req->refId, req->language, req->code);
    strcat(msg, serialized_req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), msg, response);
    REST_CodeRunResp *result = ToREST_CodeRunResp(response);
    free(response);
    return result;
}

REST_CodeRunResp *RunPythonCode(REST_CodeRunReq *req)
{
    char msg[REQ_BUFFER_SIZE] = "RUN_PYTHON_CODE:";
    char serialized_req[REQ_BUFFER_SIZE];
    sprintf(serialized_req, "refId:%s,language:%s,code:%s", req->refId, req->language, req->code);
    strcat(msg, serialized_req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), msg, response);
    REST_CodeRunResp *result = ToREST_CodeRunResp(response);
    free(response);
    return result;
}

char* createRestCompileReqJson(REST_CompileReq const req){
    cJSON *compileReq = cJSON_CreateObject();
    char *string = NULL;
    if (compileReq == NULL)
    {
        goto endCreateRestCompileReqJson;
    }

    if (cJSON_AddStringToObject(compileReq, "code", req.code) == NULL)
    {
        goto endCreateRestCompileReqJson;
    }
    if (cJSON_AddStringToObject(compileReq, "language", req.language) == NULL)
    {
        goto endCreateRestCompileReqJson;
    }
     string = cJSON_Print(compileReq);
    if (string == NULL)
    {
        perror("Failed to print compileReq.\n");
    }

endCreateRestCompileReqJson:
    cJSON_Delete(compileReq);
    return string;
}

int Test_CompileCCode(){
    REST_CompileReq compileReq;
    compileReq.code = "#include <stdio.h>\nint main(){\nprintf(\"Hello World\");\nreturn 0;\n}";
    compileReq.language = "C";
    char* json = createRestCompileReqJson(compileReq);
    REST_CompileResp *resp = CompileCCode(json);
    free(json);
    // printf("RefId: %s\n", resp->refId);

}

int codeRunLib_RunDemo()
{
    printContainersPorts();
    Test_CompileCCode();
    return 0;
}