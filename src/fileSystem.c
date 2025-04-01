#include "fileSystem.h"

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
#include <ncurses.h>

#include "defines.h"

int open_dir(DIR **dir, const char *path) {
    #ifdef DEBUG
    log_message("Trying to open %s", path);
    #endif

    if (access(path, F_OK) == -1) {
        log_message("Failed to open %s\n", path);
        return 0;
    }   
    struct stat st;
    if(stat(path, &st)) return 0; 
    if (*dir) closedir(*dir);
    if ((*dir = opendir(path)) == NULL) {
        log_message("Failed to open %s\n", path);
        perror("opendir");
        return 0;
    }
    return 1;
}

void read_usb_attribute(const char *path, char *buffer, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        buffer[0] = '\0';
        return;
    }
    
    ssize_t bytes_read = read(fd, buffer, size - 1);
    close(fd);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        if (buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 1] = '\0';
        }
    } else {
        log_message("file at %s empty", path);
        buffer[0] = '\0';
    }
}