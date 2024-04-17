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


#define CLIENT_BUFFER_SIZE 4096
// 1024 * 1024 = 1KB
#define RESPONSE_BUFFER_SIZE 4096
#define PORT 6969

void initServer(HTTP_Server *http_server, struct Route **route)
{
	// initiate HTTP_Server
	init_server(http_server, PORT);
	// registering Routes
	*route = initRoute("/", "index.html");
	addRoute(&(*route), "/about", "about.html");
	addRoute(&(*route), "/sth", "sth.html");
	addRoute(&(*route), "/chicken", "chicken.html");

	// display all available routes
	printf("\n====================================\n");
	printf("=========ALL VAILABLE ROUTES========\n");
	inorder(*route);
}

void GetMethodAndRoute(char client_msg[CLIENT_BUFFER_SIZE], char **method, char **urlRoute)
{
	char *client_http_header = strtok(client_msg, "\n");
	// printf("\n\n%s\n\n", client_http_header);
	char *header_token = strtok(client_http_header, " ");
	int header_parse_counter = 0;
	while (header_token != NULL)
	{
		switch (header_parse_counter)
		{
		case 0:
			*method = header_token;
			break;
		case 1:
			*urlRoute = header_token;
			break;
		}
		header_token = strtok(NULL, " ");
		header_parse_counter++;
	}
}

void MatchRoute(struct Route *route, char *urlRoute, char *response_data)
{
	struct Route *matchedRoute = search(route, urlRoute);

	// pages
	char templatePath[255] = "templates/";
	if (matchedRoute != NULL)
	{
		strcat(templatePath, matchedRoute->value);
		char *pageContent = render_static_file(templatePath);
		strcpy(response_data, pageContent);
		free(pageContent);
		return;
	}

	// resources like css and js
	if (strstr(urlRoute, "/static/") != NULL)
	{
		char *pageContent = render_static_file("static/index.css");
		strcpy(response_data, pageContent);
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
		else if(strstr(urlRoute, "/api/run") != NULL)
		{
			// TODO: call API handler
			printf("Running code\n");
		}

		return;
	}

	response_data = render_static_file("templates/404.html");
}

int main()
{
    InitContainersThreadpool();
	codeRunLib_RunDemo();
	return 0;

	HTTP_Server http_server;
	struct Route *route;
	int client_socket;
	initServer(&http_server, &route);

	// start listening to client
	while (1)
	{
		char client_msg[CLIENT_BUFFER_SIZE] = "";
		char *response_data = malloc(RESPONSE_BUFFER_SIZE);
		char *response = malloc(RESPONSE_BUFFER_SIZE * 2);

		// memset(response_data, '\0', RESPONSE_BUFFER_SIZE);
		client_socket = accept(http_server.socket, NULL, NULL);
		read(client_socket, client_msg, CLIENT_BUFFER_SIZE - 1);
		// printf("%s\n", client_msg);

		// parsing client socket header to get HTTP method, route
		char *method = "";
		char *urlRoute = "";
		GetMethodAndRoute(client_msg, &method, &urlRoute);
		printf("The method is %s\n", method);
		printf("The route is %s\n", urlRoute);

		MatchRoute(route, urlRoute, response_data);
		// compose response
		char http_header[4096] = "HTTP/1.1 200 OK\r\n\r\n";
		strcat(response, http_header);
		strcat(response, response_data);
		// strcat(response, "\r\n\r\n");

		// send response, close socket, free data
		// printf("Response: %s\n", response);
		ssize_t bytes_sent = send(client_socket, response, strlen(response), 0);
		if (bytes_sent == -1)
		{
			perror("Error sending response");
			close(client_socket);
			free(response_data);
			free(response);
			return -1;
		}
		close(client_socket);
		free(response_data);
		free(response);
	}
	return 0;
}