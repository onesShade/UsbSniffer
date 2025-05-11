#include <linux/limits.h>
#define _POSIX_C_SOURCE 199309L 
#define _GNU_SOURCE

#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <ncurses.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "defines.h"
#include "fileSystem.h"

void init_log() {
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
            if (strncmp(ent->d_name, devpath, strnlen(devpath, PATH_MAX)) == 0 && 
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

void init_ncurses() {
    initscr();        
    cbreak();           
    noecho();           
    keypad(stdscr, TRUE); 
    nodelay(stdscr, TRUE); 
    curs_set(0);
}

void s_strcpy(char *dest, const char *src, size_t size) {
    if(!dest || !src) {
        log_message("strcpy NULL path");
        return;
    }
    if(!snprintf(dest, size, "%s", src)) {
        log_message("strcpy error");
        return;
    }
}

int mvwprintw_centered(WINDOW* win, int y, const char* format, ...) {
    char buff[MAX_READ];
    va_list args;
    va_start(args, format);
    snprintf(buff, MAX_READ, format, args);
    va_end(args);
    int x = getmaxx(win) / 2 - strnlen(buff, MAX_READ) / 2;
    int res = mvwprintw(win, y, x, "%s", buff);
    return res;
}

void use_octal_escapes(char* str) {
    char buffer[PATH_MAX];

    int sp = 0;
    int dp = 0;
    int og_len = strnlen(str, PATH_MAX);

    for(; sp < og_len; sp++, dp++) {
        if (str[sp] == '\\' 
            && isdigit(str[sp + 1])
            && isdigit(str[sp + 2])
            && isdigit(str[sp + 3])) {
                
            char simbol = (str[sp + 1] - '0') * 64 
                        + (str[sp + 2] - '0') * 8 
                        + (str[sp + 3] - '0');
            buffer[dp] = simbol;
            sp += 3; 
        } else {
            buffer[dp] = str[sp];
        }
    }
    buffer[dp] = 0;
    s_strcpy(str, buffer, PATH_MAX);
}
