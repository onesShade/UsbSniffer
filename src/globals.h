#ifndef GLOBALS
#define GLOBALS

#include <ncurses.h>
#include "defines.h"

typedef enum {
    device_list,
} Window;

typedef struct {
    Window window;
    char device_name[PATH_MAX];
    char block_name[PATH_MAX];
    char mount_path[PATH_MAX];
    int y;
} Cursor;

typedef struct {
    int len;
} DLInfo;

typedef struct {
    int curr_y;
} ILInfo;

extern Cursor cursor;
extern DLInfo dl_info;
extern ILInfo il_info;

extern WINDOW *left_win;
extern WINDOW *right_win;
extern WINDOW *bottom_win;

typedef struct {
    const char *attribute_name;
    const char *print_prefix;
    const char *print_postfix;
} Atr_Print_arg;

void init_globals();
void reinit_windows();

void free_globals();
#endif