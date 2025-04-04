#ifndef GLOBALS
#define GLOBALS

#include <ncurses.h>
#include "defines.h"

typedef enum {
    device_list,
    storage_test_settings,
    storage_test_run
} Window;

typedef struct {
    Window window;
    char device_name[PATH_MAX];
    char block_name[PATH_MAX];
    char mount_path[PATH_MAX];
    int y;
} Selection;

typedef struct {
    int len;
} DLInfo;

typedef struct {
    int curr_y;
} ILInfo;

typedef struct {
    int curr;
    int len;
    char selectable;
} ListInfo;

extern Selection selection_lw;
extern DLInfo dl_info;
extern ILInfo il_info;

extern ListInfo left_win_info;
extern ListInfo right_win_info;

extern WINDOW *left_win;
extern WINDOW *right_win;
extern WINDOW *bottom_win;
extern WINDOW *popup_win;

typedef struct {
    const char *attribute_name;
    const char *print_prefix;
    const char *print_postfix;
} Atr_Print_arg;

void init_globals();
void reinit_windows();

void free_globals();
#endif