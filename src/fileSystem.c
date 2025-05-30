#define _POSIX_C_SOURCE 199309L 
#define _GNU_SOURCE

#include <linux/limits.h>
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

#include "../include/fileSystem.h"
#include "../include/util.h"
#include "../include/defines.h"

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

int filter_has_simbol(const char *name, const void *arg) {
    const char simbol = ((const char *)arg)[0];
    return strchr(name, simbol) != NULL;
}

int filter_prefix(const char *name, const void *arg) {
    const char *prefix = (const char *)arg;
    return strncmp(name, prefix, strnlen(prefix, MAX_READ)) == 0;
}

int filter_regular_entries(const char *name, const void* unused) {
    return name[0] != '.';
}

int find_first_matching_entry(const char* path, const FindEntryArg arg, char *result_path) {

    DIR* dir = opendir(path);
    if (!dir) {
        log_message("Failed to open dir: %s", path);
        return 0;
    }

    int found = 0;
    const struct dirent * ent;
    while ((ent = readdir(dir)) != NULL) {
        if (arg.filter_fun(ent->d_name, arg.filter_arg)) {
            s_strcpy(result_path, ent->d_name, PATH_MAX);
            found = 1;
            break;
        }
    }

    closedir(dir);
    return found;
}

int traverse_path(const char *base_path, const FindEntryArg * const arg_array, char *final_path) {
        
    char current_path[PATH_MAX];
    char temp_path[PATH_MAX];
    s_strcpy(current_path, base_path, PATH_MAX);

    for (int i = 0; arg_array[i].filter_fun; i++) {
#ifdef DEBUG
        log_message("%s", current_path);
#endif

        char next_entry[PATH_MAX];
        if (!find_first_matching_entry(current_path, arg_array[i], next_entry)) {
            return 0;
        }
        if (strnlen(current_path, PATH_MAX) + strnlen(next_entry, PATH_MAX) + 1 >= PATH_MAX) {
            log_message("traverse_path error: path too big");
            return 0;
        }
        snprintf(temp_path, PATH_MAX, "%s/%s", current_path, next_entry);
        s_strcpy(current_path, temp_path, PATH_MAX);
    }

    s_strcpy(final_path, current_path, PATH_MAX);
    return 1;
}

void extract_top_dir(const char *path, char *output) {
    if (!path || !output) return;

    const char *last_slash = strrchr(path, '/');
    
    if (!last_slash) {
        s_strcpy(output, path, PATH_MAX);
    } else {
        s_strcpy(output, last_slash + 1, PATH_MAX);
    }
    return;
}
