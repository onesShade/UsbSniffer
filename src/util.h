#ifndef UTIL
#define UTIL

#include <stddef.h>

#define LOG_FILE "app.log"

void log_init();
void log_message(const char *format, ...);
void msleep(int milliseconds);
void read_file_content(const char *path, char *buffer, size_t size);

#endif