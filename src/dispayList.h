#ifndef DISPLAY_LIST
#define DISPLAY_LIST

#include "vector.h"
#include <ncurses.h>
#include "defines.h"

typedef struct {
    unsigned char centered : 1;
    unsigned char unselectable : 1;
} DLEP;

typedef struct {
    unsigned char hide_selection : 1;
} DLRP;

typedef enum {
    DLEP_NONE = 0,
    DLEP_CENTERED = 1,
    DLEP_UNSELECTABLE = 2,
} DLEPe;

typedef enum {
    DLRP_NONE = 0,
    DLRP_HIDE_SELECTION = 1,
} DLRPe;

typedef struct {
    Vector* entryes;
    size_t selected;
    char selectable;
    char invisible;
    char horizontal_shift;
    int x;
    int y;
} DispayList;

typedef struct {
    char body[MAX_DL_STR];
    DLEP prop;
} DLE;   

DispayList* dl_init(const char selectable, const char horizontal_shift, const int x, const int y);

void dl_free(DispayList* dl);

int dl_iterate(DispayList* dl, int move);

void dl_draw(const DispayList* dl, WINDOW* win, const DLRPe dlrp);

void dl_clear(DispayList* dl);

DLE* dl_add_entry(DispayList* dl, DLEPe dlep, const char *format, ...);

char* dl_get_selected(DispayList* dl);

void dl_set_pos(DispayList* dl, int x, int y);

void dl_sort_natural(DispayList* dl);

void dl_reset_sel_pos(DispayList* dl);

#endif