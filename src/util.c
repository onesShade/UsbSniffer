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

#include "defines.h"

void log_init() {
    unlink(LOG_FILE);
    log_message("Program started");
}

void log_message(const char *format, ...) {
    FILE *log_file = fopen(LOG_FILE, "a");  // Open in append mode
    if (!log_file) return;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_file, "[%s] ", time_buf);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
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

int open_dir(DIR **dir, const char *path) {
    #ifdef DEBUG
    log_message("Trying to open %s", path);
    #endif

    if (access(path, F_OK) == -1) {
        log_message("Failed to open %s\n", path);
        return 0;
    }   

    if (*dir) closedir(*dir);
    if ((*dir = opendir(path)) == NULL) {
        log_message("Failed to open %s\n", path);
        perror("opendir");
        return 0;
    }
    return 1;
}

int is_usb_device(const char *name) {
    if (strchr(name, ':') != NULL) return 0;
    if (strncmp(name, "usb", 3) == 0) return 0;
    if (strchr(name, '-') != NULL) return 1;
    return 0;
}

int is_storage_device(const char *devpath) {
    char path[PATH_MAX];
    char buffer[MAX_READ];
    char iface_path[PATH_MAX];

    snprintf(path, sizeof(path), "%s%s/bDeviceClass", SYSFS_USB_DEVICES, devpath);
    read_usb_attribute(path, buffer, sizeof(buffer));
    if (strstr("08", buffer)) {
        return 1;
    }
    
    DIR *dir;
    struct dirent *ent;
    snprintf(path, sizeof(path), "%s%s", SYSFS_USB_DEVICES, devpath);
    
    if ((dir = opendir(path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, devpath, strlen(devpath)) == 0 && 
                strchr(ent->d_name, ':') != NULL) {
                
                snprintf(iface_path, sizeof(iface_path), "%s/%s/bInterfaceClass", path, ent->d_name);
                read_usb_attribute(iface_path, buffer, sizeof(buffer));
                if (strstr("08", buffer)) {
                    closedir(dir);
                    return 1;
                }
            }
        }
        if (dir) closedir(dir);
    }
    return 0;    
}