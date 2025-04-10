#ifndef UTIL
#define UTIL

#include <dirent.h>
#include <stddef.h>
#include <ncurses.h>

void init_log();
void init_ncurses();

void log_message(const char *format, ...);
void msleep(int milliseconds);
int is_usb_device(const char *name);
int is_storage_device(const char *devpath);
void s_strcpy(char *dest, const char *src, size_t size);
int mvwprintw_centered(WINDOW* win, int y, const char* format, ...);
#endif