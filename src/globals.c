#include "globals.h"
#include "dispayList.h"
#include <ncurses.h>

Selection selection_lw;

WINDOW *left_win;
WINDOW *right_win;
WINDOW *bottom_win;
WINDOW *popup_win;

DispayList* devices_dl;
DispayList* atr_dl;
DispayList* mount_point_dl;
DispayList* test_size_sel_dl;
DispayList* test_passes_dl;

int update_cycle_counter;
char is_open;

void init_globals() {
    is_open = TRUE;
    update_cycle_counter = 2;
    left_win = NULL;
    right_win = NULL;
    bottom_win = NULL;
    popup_win = NULL;
    devices_dl = dl_init(TRUE, FALSE, 1, 1);
    atr_dl = dl_init(FALSE, FALSE, 1, 1);

    mount_point_dl = dl_init(TRUE, FALSE, 1, 1);
    mount_point_dl->selected = 2;

    test_size_sel_dl = dl_init(TRUE, 8, 4, 1);

    const char* data_sizes[] = {"0001", "0016", "0064", "0128", "0512", "1024", NULL};
    for(int i = 0; data_sizes[i]; i++) {
        dl_add_entry(test_size_sel_dl, DLEP_NONE, data_sizes[i]);
    }
    dl_add_entry(test_size_sel_dl, DLEP_UNSELECTABLE, "MB");


    test_passes_dl = dl_init(TRUE, 8, 4, 1);
    const char* passes_n[] = {"0001", "0003", "0005", "0010", "0025", "00100", NULL};
    for(int i = 0; passes_n[i]; i++) {
        dl_add_entry(test_passes_dl, DLEP_NONE, passes_n[i]);
    }
    dl_add_entry(test_passes_dl, DLEP_UNSELECTABLE, "TIMES");

    reinit_windows();
}

void reinit_windows() {
    if(left_win) werase(left_win);
    left_win = newwin(LINES - 2, COLS / 3, 1, 0);
    if(right_win) werase(left_win);
    right_win = newwin(LINES - 2, COLS / 3 * 2, 1, COLS / 3);
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
    dl_free(test_passes_dl);
}