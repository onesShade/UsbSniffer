#ifndef GLOBALS
#define GLOBALS

#include <ncurses.h>
#include "defines.h"
#include "dispayList.h"
#include <linux/limits.h>

typedef enum {
    device_list,
    storage_test_settings,
    storage_test_run,
    storage_test_results,
} Window;

typedef struct {
    Window window;
    char device_name[PATH_MAX];
    char block_name[PATH_MAX];
    char block_path[PATH_MAX];
} Selection;

extern Selection selection;
extern int update_cycle_counter;
extern char is_open;

extern WINDOW *left_win;
extern WINDOW *right_win;
extern WINDOW *bottom_win;
extern WINDOW *popup_win;

extern DispayList* devices_dl;
extern DispayList* atr_dl;
extern DispayList* mount_point_dl;
extern DispayList* test_size_sel_dl;
extern DispayList* test_block_size_sel_dl;
extern DispayList* test_passes_sel_dl;
extern DispayList* test_mode_sel_dl;
extern DispayList* test_screen_dl;
extern DispayList* bottom_line_dl;

void update_sel_dls();

void init_globals();
void reinit_windows();

void free_globals();
#endif