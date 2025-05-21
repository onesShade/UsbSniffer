#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <dirent.h>
#include <stddef.h>
#include "dispayList.h"

typedef struct {
    int (*filter_fun)(const char *name, const void *filter_arg);
    const void* filter_arg;
} FindEntryArg;

typedef struct {
    const char *attribute_name;
    const char *print_prefix;
    const char *print_postfix;
} Atr_Print_arg;

int open_dir(DIR **dir, const char *path);
void read_usb_attribute(const char *path, char *buffer, size_t size);

int filter_has_simbol(const char *name, const void *arg);
int filter_prefix(const char *name, const void *arg);
int filter_regular_entries(const char *name, const void *);

int find_first_matching_entry(const char* path, FindEntryArg arg, char *result_path);
int traverse_path(const char *base_path, const FindEntryArg* arg_array, char *final_path);

void extract_top_dir(const char *path, char *output);

int print_attribute_value(const char* dir,const Atr_Print_arg arg, DispayList *dl);
#endif