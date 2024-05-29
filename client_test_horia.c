#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ServerRequest
{
    char *language;
    char *code;
} ServerRequest;

void initializeRequest(ServerRequest *request, const char *language, const char *code) {
    request->language = strdup(language);
    request->code = strdup(code);
}

void freeRequest(ServerRequest *request) {
    free(request->language);
    free(request->code);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <language> <code> [--file]\n", argv[0]);
        return 1;
    }

    const char *language = argv[1];
    const char *code = argv[2];
    int isFile = argc == 4 && strcmp(argv[3], "--file") == 0;

    ServerRequest request;
    if (isFile) {
        FILE *file = fopen(code, "r");
        if (!file) {
            perror("Failed to open file");
            return 1;
        }
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char *buffer = malloc(length + 1);
        if (buffer) {
            fread(buffer, 1, length, file);
            buffer[length] = '\0';
        }
        fclose(file);
        initializeRequest(&request, language, buffer);
        free(buffer);
    } else {
        initializeRequest(&request, language, code);
    }

    printf("Running command: run %s 1 2 3\n", request.code);
  

    freeRequest(&request);

    return 0;
}