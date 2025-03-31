#ifndef UTIL
#define UTIL

#include <dirent.h>
#include <stddef.h>

void init_log();
void init_ncurses();

void log_message(const char *format, ...);
void msleep(int milliseconds);
void read_usb_attribute(const char *path, char *buffer, size_t size);
int open_dir(DIR **dir, const char *path);
int is_usb_device(const char *name);
int is_storage_device(const char *devpath);

#endif