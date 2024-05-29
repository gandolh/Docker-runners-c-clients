/**
 * Examples of files to run:
 * - C: examples/example.c
 * - Rust: examples/example.rs
 * - Python: examples/example.py
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        return "Python";
    case 2:
        return "Rust";
    case 3:
        return "C";
    default:
        return NULL;
    }
}

int main()
{
    char inputBuffer[10];
    char code[256];
    char fileOption[10];

    while (1)
    {
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

        freeRequest(&request);
    }

    return 0;
}