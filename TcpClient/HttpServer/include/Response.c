#include "Response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Logger.h"

void render_static_file(char *fileName, char *response_data, int readSize)
{
	FILE *file = fopen(fileName, "r");
	if (file == NULL)
	{
		write_log("Failed to open file");
		return;
	}

	size_t bytesRead = fread(response_data, 1, readSize, file);
	if (bytesRead < readSize)
	{
		if (feof(file))
			write_log("End of file reached.\n");
		else if (ferror(file))
			write_log("Error reading file");
	}

	fclose(file);
}