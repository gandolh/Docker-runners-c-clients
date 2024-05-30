#include "Response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Logger.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#define HEADER_BUFFER_SIZE 20
void render_static_file(char const *fileName, int *client_socket)
{
	char http_header[HEADER_BUFFER_SIZE] = "HTTP/1.1 200 OK\r\n\r\n";
	// Get the size of the file
	struct stat st;
	if (stat(fileName, &st) == -1)
	{
		perror("stat failed");
		return;
	}
	size_t file_size = st.st_size;

	// Allocate a buffer for the file contents
	char *file_contents = malloc(file_size + 1);
	if (file_contents == NULL)
	{
		perror("malloc failed");
		return;
	}

	// Open the file
	FILE *file = fopen(fileName, "rb");
	if (file == NULL)
	{
		perror("fopen failed");
		free(file_contents);
		return;
	}

	// Read the entire file into the buffer
	size_t bytes_read = fread(file_contents, 1, file_size, file);
	if (bytes_read != file_size)
	{
		perror("fread failed");
		free(file_contents);
		fclose(file);
		return;
	}

	// Null-terminate the buffer
	file_contents[file_size] = '\0';

	// Close the file
	fclose(file);

	// Concatenate the header and the file contents
	size_t header_length = strlen(http_header);
	char *response = malloc(header_length + file_size + 1);
	if (response == NULL)
	{
		perror("malloc failed");
		free(file_contents);
		return;
	}
	memcpy(response, http_header, header_length);
	memcpy(response + header_length, file_contents, file_size + 1); // +1 to copy the null terminator

	// Send the response
	ssize_t bytes_sent = send(*client_socket, response, header_length + file_size, 0);

	// Free the buffers
	free(response);
	free(file_contents);
}