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

char *createRestCompileReqJson(CodeRunnerCompileRequest const req)
{
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
    if (cJSON_AddStringToObject(compileReq, "type", req.type) == NULL)
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

char *createRestRunReqJson(CodeRunnerRunRequest const req)
{
    cJSON *runReq = cJSON_CreateObject();
    char *string = NULL;
    if (runReq == NULL)
    {
        goto endCreateRestRunReqJson;
    }

    if (cJSON_AddStringToObject(runReq, "filename", req.filename) == NULL)
    {
        goto endCreateRestRunReqJson;
    }
    if (cJSON_AddStringToObject(runReq, "language", req.language) == NULL)
    {
        goto endCreateRestRunReqJson;
    }
    if (cJSON_AddStringToObject(runReq, "type", req.type) == NULL)
    {
        goto endCreateRestRunReqJson;
    }
    if (cJSON_AddStringToObject(runReq, "code", req.code) == NULL)
    {
        goto endCreateRestRunReqJson;
    }
    string = cJSON_Print(runReq);
    if (string == NULL)
    {
        perror("Failed to print runReq.\n");
    }

endCreateRestRunReqJson:
    cJSON_Delete(runReq);
    return string;
}

CodeRunnerResponse *deserializeCodeRunnerResponse(char *response)
{
    cJSON *root = cJSON_Parse(response);

    if (!root)
    {
        write_log("error: %s\n", cJSON_GetErrorPtr());
        return NULL;
    }

    CodeRunnerResponse *resp = malloc(sizeof(CodeRunnerResponse));
    if (!resp)
    {
        perror("malloc");
        return NULL;
    }

    cJSON *status = cJSON_GetObjectItem(root, "status");
    cJSON *filename = cJSON_GetObjectItem(root, "filename");
    cJSON *stdout = cJSON_GetObjectItem(root, "stdout");
    cJSON *stderr = cJSON_GetObjectItem(root, "stderr");

    if (!cJSON_IsString(status) || !cJSON_IsString(filename) || !cJSON_IsString(stdout) || !cJSON_IsString(stderr))
    {
        write_log("error: one or more fields are not strings\n");
        free(resp);
        return NULL;
    }

    resp->status = strdup(status->valuestring);
    resp->filename = strdup(filename->valuestring);
    resp->stdout = strdup(stdout->valuestring);
    resp->stderr = strdup(stderr->valuestring);

    cJSON_Delete(root);

    return resp;
}

CodeRunnerResponse *CompileCCode(char *const code)
{
    CodeRunnerCompileRequest req;
    req.code = code;
    req.language = "C";
    req.type = "COMPILE";
    char *json = createRestCompileReqJson(req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), json, response);
    CodeRunnerResponse *resp = deserializeCodeRunnerResponse(response);
    free(response);
    free(json);
    write_log("Response status: %s\n", resp->status);
    return resp;
}

CodeRunnerResponse *CompileRustCode(char *const code)
{
    CodeRunnerCompileRequest req;
    req.code = code;
    req.language = "RUST";
    req.type = "COMPILE";
    char *json = createRestCompileReqJson(req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), json, response);
    CodeRunnerResponse *resp = deserializeCodeRunnerResponse(response);
    free(response);
    free(json);
    write_log("Response status: %s\n", resp->status);
    return resp;
}

CodeRunnerResponse *RunCCode(char *const filename)
{
    CodeRunnerRunRequest req;
    req.filename = filename;
    req.language = "C";
    req.type = "RUN";
    char *json = createRestRunReqJson(req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), json, response);
    CodeRunnerResponse *resp = deserializeCodeRunnerResponse(response);
    free(response);
    free(json);
    write_log("Response status: %s\n", resp->status);
    return resp;
}

CodeRunnerResponse *RunRustCode(char *const filename)
{
    CodeRunnerRunRequest req;
    req.filename = filename;
    req.code = "";
    req.language = "C";
    req.type = "RUN";
    char *json = createRestRunReqJson(req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), json, response);
    CodeRunnerResponse *resp = deserializeCodeRunnerResponse(response);
    free(response);
    free(json);
    write_log("Response status: %s\n", resp->status);
    return resp;
}

CodeRunnerResponse *RunPythonCode(char *const code)
{
    CodeRunnerRunRequest req;
    req.code = code;
    req.filename = "";
    req.language = "PYTHON";
    req.type = "RUN";
    char *json = createRestRunReqJson(req);
    char *response = malloc(RES_BUFFER_SIZE);
    SendRequest(getAvailablePort(), json, response);
    CodeRunnerResponse *resp = deserializeCodeRunnerResponse(response);
    free(response);
    free(json);
    write_log("Response python status: %s\n", resp->status);
    return resp;
}

int Test_CompileAndRunCCode()
{
    CodeRunnerResponse *resp = CompileCCode("#include <stdio.h>\nint main(){\nprintf(\"Hello World\");\nreturn 0;\n}");
    // printf("RefId: %s\n", resp->refId);
    RunCCode(resp->filename);
}

int Test_CompileAndRunRustCode()
{
    CodeRunnerResponse *resp = CompileRustCode("fn main() {\n"
                                               "    println!(\"Hello, world!\");\n"
                                               "}\n");
    RunRustCode(resp->filename);
}

int Test_RunPythonCode()
{
    CodeRunnerResponse *resp = RunPythonCode("print(\"Hello World\")");
}

int codeRunLib_RunDemo()
{
    printContainersPorts();
    Test_CompileAndRunCCode();
    Test_CompileAndRunRustCode();
    Test_RunPythonCode();
    return 0;
}