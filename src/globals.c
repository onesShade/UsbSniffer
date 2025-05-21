#include <ncurses.h>
#include <stdbool.h>

#include "../include/globals.h"
#include "../include/dispayList.h"

Selection selection;

WINDOW *left_win;
WINDOW *right_win;
WINDOW *bottom_win;
WINDOW *popup_win;

DispayList* devices_dl;
DispayList* atr_dl;
DispayList* mount_point_dl;
DispayList* test_size_sel_dl;
DispayList* test_block_size_sel_dl;
DispayList* test_passes_sel_dl;
DispayList* test_mode_sel_dl;
DispayList* test_screen_dl;
DispayList* bottom_line_dl;

int update_cycle_counter;
char is_open;

void init_globals() {
    is_open = TRUE;
    update_cycle_counter = 2;
    left_win = NULL;
    right_win = NULL;
    bottom_win = NULL;
    popup_win = NULL;

    selection.device_name[0] = '\0';

    devices_dl = dl_init(DLP_SELECTABLE, 1, 1);
    atr_dl = dl_init(DLP_NONE, 1, 1);

    bottom_line_dl = dl_init(DLP_HORIZONTAL, 0, 0);
    bottom_line_dl->horizontal_shift = 16;

    mount_point_dl = dl_init(DLP_SELECTABLE, 1, 1);
    test_mode_sel_dl = dl_init( DLP_HORIZONTAL | DLP_SELECTABLE, 4, 1);
    test_size_sel_dl = dl_init(DLP_HORIZONTAL | DLP_SELECTABLE, 4, 1);
    test_passes_sel_dl = dl_init(DLP_HORIZONTAL | DLP_SELECTABLE, 4, 1);
    test_block_size_sel_dl = dl_init(DLP_HORIZONTAL |DLP_SELECTABLE , 4, 1);
    test_screen_dl = dl_init(DLP_NONE, 1, 1);

    const char* mode_str[] = {"WS", "RS", "RR", NULL};
    dl_add_entry(test_mode_sel_dl, DLEP_UNSELECTABLE, "mode:");
    for(int i = 0; mode_str[i]; i++) {
        dl_add_entry(test_mode_sel_dl, DLEP_NONE, mode_str[i]);
    }
    dl_reset_sel_pos(test_mode_sel_dl);

    const char* data_sizes[] = {"0001", "0016", "0064", "0128", "0512", "1024", NULL};
    dl_add_entry(test_size_sel_dl, DLEP_UNSELECTABLE, "size:");
    for(int i = 0; data_sizes[i]; i++) {
        dl_add_entry(test_size_sel_dl, DLEP_NONE, data_sizes[i]);
    }
    dl_add_entry(test_size_sel_dl, DLEP_UNSELECTABLE, "MB");
    dl_reset_sel_pos(test_size_sel_dl);

    const char* block_sizes[] = {"001", "004", "008", "016", "032", "064", NULL};
    dl_add_entry(test_block_size_sel_dl, DLEP_UNSELECTABLE, "block:");
    for(int i = 0; block_sizes[i]; i++) {
        dl_add_entry(test_block_size_sel_dl, DLEP_NONE, data_sizes[i]);
    }
    dl_add_entry(test_block_size_sel_dl, DLEP_UNSELECTABLE, "KB");
    dl_reset_sel_pos(test_block_size_sel_dl);

    update_sel_dls();
    reinit_windows();
}

void reinit_windows() {
    if(left_win) werase(left_win);
    left_win = newwin(LINES - 1, COLS / 3, 0, 0);
    if(right_win) werase(left_win);
    right_win = newwin(LINES - 1, COLS / 3 * 2, 0, COLS / 3);
    if(bottom_win) werase(left_win);
    bottom_win = newwin(1, COLS, LINES - 1, 1);
    if(popup_win) werase(popup_win);
    popup_win = newwin( LINES * 8 / 10, COLS * 8 / 10, LINES / 10, COLS / 10);
}

void free_globals() {
    delwin(left_win);
    delwin(right_win);
    delwin(bottom_win);
    delwin(popup_win);
    endwin();  

    dl_free(devices_dl);
    dl_free(atr_dl);
    dl_free(mount_point_dl);
    dl_free(test_size_sel_dl);
    dl_free(test_passes_sel_dl);
    dl_free(test_mode_sel_dl);
    dl_free(test_block_size_sel_dl);
}