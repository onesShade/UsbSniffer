#define _POSIX_C_SOURCE 199309L
#include "util.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

void log_init() {
    unlink(LOG_FILE);
}

void log_message(const char *format, ...) {
    FILE *log_file = fopen(LOG_FILE, "a");  // Open in append mode
    if (!log_file) return;
    
    // Get current time
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Write timestamp
    fprintf(log_file, "[%s] ", time_buf);
    
    // Process variable arguments
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    // Ensure newline and flush
    fputc('\n', log_file);
    fclose(log_file);
}

void msleep(int milliseconds) {
    struct timespec ts = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (milliseconds % 1000) * 1000000
    };
    nanosleep(&ts, NULL);
}

void read_file_content(const char *path, char *buffer, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        log_message("fopen err %s", path);
        buffer[0] = '\0';
        return;
    }
    
    ssize_t bytes_read = read(fd, buffer, size - 1);
    close(fd);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        // Remove trailing newline if present
        if (buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 1] = '\0';
        }
    } else {
        log_message("file at %s empty", path);
        buffer[0] = '\0';
    }
}
