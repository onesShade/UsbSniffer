#ifndef DISPLAY_LIST
#define DISPLAY_LIST

#include "vector.h"
#include <ncurses.h>

typedef struct {
    Vector* entryes;
    size_t selected;
    char selectable;
    int x;
    int y;
} DispayList;

DispayList* dl_init(char selectable, int x, int y);

void dl_free(DispayList* dl);

void dl_iterate(DispayList* dl, int move);

void dl_draw(DispayList* dl, WINDOW* window);

void dl_clear(DispayList* dl);

void dl_add_entry(DispayList* dl, const char *format, ...);

char* dl_get_selected(DispayList* dl);

void dl_set_pos(DispayList* dl, int x, int y);

#endif