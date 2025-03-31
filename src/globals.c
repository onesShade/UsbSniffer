#include "globals.h"

Cursor cursor;
DLInfo dl_info;
ILInfo il_info;

WINDOW *left_win;
WINDOW *right_win;
WINDOW *bottom_win;

void init_globals() {
    left_win = NULL;
    right_win = NULL;
    bottom_win = NULL;
    reinit_windows();
}

void reinit_windows() {
    if(left_win) werase(left_win);
    left_win = newwin(LINES - 2, COLS / 2, 1, 0);
    if(right_win) werase(left_win);
    right_win = newwin(LINES - 2, COLS / 2, 1, COLS / 2);
    if(bottom_win) werase(left_win);
    bottom_win = newwin(1, COLS, LINES - 1, 1);
}

void free_globals() {
    delwin(left_win);
    delwin(right_win);
    delwin(bottom_win);
    endwin();  
}