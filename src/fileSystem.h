#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <dirent.h>
#include <stddef.h>

typedef struct {
    int (*filter_fun)(const char *name, void *filter_arg);
    void* filter_arg;
} FindEntryArg;

int open_dir(DIR **dir, const char *path);
void read_usb_attribute(const char *path, char *buffer, size_t size);

#endif