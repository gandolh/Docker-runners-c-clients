/**
 * TODO:
 * Este doar un tip de comanda, si anume RUN. Se pune totul in ServerRequest.
 *  language e cu alege 1 2 sau 3 si pune in struct (1 = C  2 = RUST sau 3 = PYTHON).
 * la cod sa aleaga daca zice locatia unui fisier (si atunci citeste de acolo codul si il pune in struct) sau scrie de mana codul in terminal
 * sa printeze struct-ul sa vedem ca e totul ok
 */

typedef struct ServerRequest
{
    char *language;
    char *code;
} ServerRequest;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

#define CLIENT_BUFFER_SIZE 4096
#define SERVER_BUFFER_SIZE 4096 + 20
#define PORT 6969
#define MAX_CONSOLE_COMMAND_SIZE 50

int main(int argc, char *argv[])
{

    // Create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Specify server details

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    printf("PRESS -h for help\n");
    while (1)
    {
        char str[MAX_CONSOLE_COMMAND_SIZE];
        fgets(str, MAX_CONSOLE_COMMAND_SIZE, stdin);
        // remove endline
        str[strlen(str) - 1] = '\0';
        printf("You entered: %s\n", str);
    }

    return 0;
}