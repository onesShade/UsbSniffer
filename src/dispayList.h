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

DispayList* dl_init(char selectable);

void dl_iterate(DispayList* dl, int move);

void draw(WINDOW* window);

void dl_clear(DispayList* dl);

void dl_add_entry(DispayList* dl, const char *format, ...);

char* dl_get_selected(DispayList* dl);

#endif