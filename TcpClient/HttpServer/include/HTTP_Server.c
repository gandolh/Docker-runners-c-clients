#include "HTTP_Server.h"
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include "Logger.h"

void init_server(HTTP_Server *http_server, int port)
{
	http_server->port = port;

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		write_log("Failed to create socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
	{
		write_log("Failed to bind socket");
		exit(EXIT_FAILURE);
	}
	write_log("bind socket to port %d", port);

	if (listen(server_socket, 5) == -1)
	{
		write_log("Failed to listen on socket");
		exit(EXIT_FAILURE);
	}
	write_log("Listening on port %d", port);

	http_server->socket = server_socket;
	write_log("HTTP Server Initialized\nPort: %d\n", http_server->port);
}