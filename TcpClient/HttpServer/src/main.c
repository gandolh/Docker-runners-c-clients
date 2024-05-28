#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "HTTP_Server.h"
#include "Routes.h"
#include "Response.h"
#include "Code_Run_Lib.h"
#include "REST_Api_Handler.h"
#include "Logger.h"

#define CLIENT_BUFFER_SIZE 4096
#define RESPONSE_DATA_BUFFER_SIZE 4096
#define HEADER_BUFFER_SIZE 20
#define RESPONSE_BUFFER_SIZE RESPONSE_DATA_BUFFER_SIZE + HEADER_BUFFER_SIZE
#define PORT 6969

void MatchRoute(struct Route *route, char *urlRoute, char *response_data)
{
	struct Route *matchedRoute = search(route, urlRoute);

	// pages
	char templatePath[255] = "templates/";
	if (matchedRoute != NULL)
	{
		strcat(templatePath, matchedRoute->value);
		render_static_file(templatePath, response_data, RESPONSE_DATA_BUFFER_SIZE);
		write_log("response_data: %s\n", response_data);
		return;
	}

	// resources like css and js
	if (strstr(urlRoute, "/static/") != NULL)
	{
		render_static_file("static/index.css", response_data, RESPONSE_DATA_BUFFER_SIZE);
		return;
	}

	// Rest API endpoints
	if (strstr(urlRoute, "/api/") != NULL)
	{
		if (strstr(urlRoute, "/api/compile") != NULL)
		{
			// TODO: call api handler
			printf("Compiling code\n");
		}
		else if (strstr(urlRoute, "/api/run") != NULL)
		{
			// TODO: call API handler
			printf("Running code\n");
		}

		return;
	}

	render_static_file("templates/404.html", response_data, RESPONSE_DATA_BUFFER_SIZE);
}

int main()
{
	create_log_file();
	InitContainersThreadpool();
	// just for testing
	// codeRunLib_RunDemo();
	// return 0;

	HTTP_Server http_server;
	struct Route *route;
	int client_socket;
	// initiate HTTP_Server
	init_server(&http_server, PORT);
	// registering Routes
	route = initRoute("/", "index.html");
	addRoute(&route, "/about", "about.html");
	addRoute(&route, "/sth", "sth.html");
	addRoute(&route, "/chicken", "chicken.html");

	// display all available routes
	printf("\n====================================\n");
	printf("=========ALL VAILABLE ROUTES========\n");
	inorder(route);

	// request got from client
	char client_msg[CLIENT_BUFFER_SIZE] = "";
	// response data to be sent to client. No header included
	char *response_data = malloc(RESPONSE_DATA_BUFFER_SIZE);
	// response data with header
	char *response = malloc(RESPONSE_BUFFER_SIZE);
	// start listening to client
	while (1)
	{
		memset(client_msg, 0, CLIENT_BUFFER_SIZE);
		memset(response_data, 0, RESPONSE_DATA_BUFFER_SIZE);
		memset(response, 0, RESPONSE_BUFFER_SIZE);

		client_socket = accept(http_server.socket, NULL, NULL);
		read(client_socket, client_msg, CLIENT_BUFFER_SIZE - 1);

		// parsing client socket header to get HTTP method, route
		char *method = "";
		char *urlRoute = "";

		char *client_http_header = strtok(client_msg, "\n");

		printf("\n\n%s\n\n", client_http_header);

		char *header_token = strtok(client_http_header, " ");

		int header_parse_counter = 0;

		while (header_token != NULL)
		{

			switch (header_parse_counter)
			{
			case 0:
				method = header_token;
				break;
			case 1:
				urlRoute = header_token;
				break;
			}
			header_token = strtok(NULL, " ");
			header_parse_counter++;
		}

		printf("The method is %s\n", method);
		printf("The route is %s\n", urlRoute);

		// match route
		MatchRoute(route, urlRoute, response_data);
		// compose response
		char http_header[HEADER_BUFFER_SIZE] = "HTTP/1.1 200 OK\r\n\r\n";
		snprintf(response, RESPONSE_BUFFER_SIZE, "%s%s", http_header, response_data);
		// write_log("Response: %s\n", response);
		ssize_t bytes_sent = send(client_socket, response, RESPONSE_BUFFER_SIZE, 0);
		if (bytes_sent == -1)
		{
			perror("Error sending response");
			close(client_socket);
			break;
		}
		close(client_socket);
	}
	free(response_data);
	free(response);
	return 0;
}