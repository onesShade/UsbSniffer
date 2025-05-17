#include "globals.h"
#include "dispayList.h"
#include <ncurses.h>
#include <stdbool.h>
#include <string.h>

Selection selection_lw;

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

int update_cycle_counter;
char is_open;

void update_sel_dls() {
    const char* passes_n[] = {"0001", "0003", "0005", "0010", "0025", "0100","0500",NULL};
    const char* passes_n_rand[] = {"0100", "0250", "0500", "1000", "2000", "5000","10000",NULL};

    dl_clear(test_passes_sel_dl);
    dl_clear(test_passes_sel_dl);

    dl_add_entry(test_passes_sel_dl, DLEP_UNSELECTABLE, "passes: "); 
    if (strncmp("RR", dl_get_selected(test_mode_sel_dl), MAX_READ) == 0) {
        for (int i = 0; passes_n_rand[i]; i++) {
            dl_add_entry(test_passes_sel_dl, DLEP_NONE, passes_n_rand[i]);
        }
        test_block_size_sel_dl->invisible = FALSE;
    } else {
        for (int i = 0; passes_n[i]; i++) {
            dl_add_entry(test_passes_sel_dl, DLEP_NONE, passes_n[i]);
        }
        test_block_size_sel_dl->invisible = TRUE;
    }
    dl_add_entry(test_passes_sel_dl, DLEP_UNSELECTABLE, "TIMES");
    dl_reset_sel_pos(test_passes_sel_dl);

}

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
    test_mode_sel_dl = dl_init(TRUE, 8, 4, 1);
    test_size_sel_dl = dl_init(TRUE, 8, 4, 1);
    test_passes_sel_dl = dl_init(TRUE, 8, 4, 1);
    test_block_size_sel_dl = dl_init(TRUE, 8, 4, 1);
    test_screen_dl = dl_init(FALSE, 0, 1, 1);

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