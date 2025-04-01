#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <dirent.h>
#include <stddef.h>

int open_dir(DIR **dir, const char *path);
void read_usb_attribute(const char *path, char *buffer, size_t size);

#endif