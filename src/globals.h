#ifndef GLOBALS
#define GLOBALS

#include <ncurses.h>
#include "defines.h"
#include "dispayList.h"

typedef enum {
    device_list,
    storage_test_settings,
    storage_test_run
} Window;

typedef struct {
    Window window;
    char device_name[PATH_MAX];
    char block_name[PATH_MAX];
    char block_path[PATH_MAX];
    char mount_path[PATH_MAX];
} Selection;

extern Selection selection_lw;
extern int update_cycle_counter;
extern char is_open;

extern WINDOW *left_win;
extern WINDOW *right_win;
extern WINDOW *bottom_win;
extern WINDOW *popup_win;

extern DispayList* devices_dl;
extern DispayList* atr_dl;
extern DispayList* mount_point_dl;
typedef struct {
    const char *attribute_name;
    const char *print_prefix;
    const char *print_postfix;
} Atr_Print_arg;

void init_globals();
void reinit_windows();

void free_globals();
#endif