#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

// Function to create a log file
void create_log_file();

// Function to write a log message
void write_log(const char *format, ...);

#endif // LOGGER_H