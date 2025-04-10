#include <stddef.h>
#define _POSIX_C_SOURCE 199309L 
#define _GNU_SOURCE

#include "dispayList.h"
#include "util.h"
#include "vector.h"
#include <linux/limits.h>
#include <ncurses.h>
#include <stdlib.h>
#include "defines.h"
#include <string.h>
#include <ctype.h>



DispayList* dl_init(const char selectable, const char horizontal_shift, const int x, const int y) {
    DispayList* dl = malloc(sizeof(DispayList));
    dl->selectable = selectable;
    dl->selected = 0;
    dl->x = x;
    dl->y = y;
    dl->horizontal_shift = horizontal_shift;
    dl->entryes = vector_init(sizeof(DLE), 16);
    return dl;
}

int dl_iterate(DispayList* dl, int move) {
    if(!dl->entryes->size) {
        return 0;
    }
    if((move != 1 && move != -1) || !dl->selectable) {
        log_message("dl_iterate exception");
        return 0;
    }
    size_t start_pos = dl->selected;
    do {
        dl->selected += move;
        dl->selected += dl->entryes->size;
        dl->selected = dl->selected % dl->entryes->size;
    if(dl->selected == start_pos) {
#ifdef DEBUG
        log_message("infinite loop at dl_iterate");
#endif
        return 0;
    }

    } while(((const DLE*)vector_at(dl->entryes, dl->selected))->prop.unselectable);
    return 1;
}

void dl_clear(DispayList* dl) {
    vector_clear(dl->entryes);
}

void dl_add_entry(DispayList* dl, DLEProperties dlep, const char *format, ...) {
    char buff[PATH_MAX];
    int size = 0;
    va_list args;
    va_start(args, format);
    size = vsprintf(buff, format, args);
    va_end(args);
    buff[MAX_DL_STR] = dlep;
    if (size >= MAX_DL_STR) {
        log_message("DL entry size overflow on %s", buff + 1);
        return;
    }
    vector_push_back(dl->entryes, buff);
}

void dl_draw(const DispayList* dl, WINDOW* win, const DLRProperties dlrp) {

    const DLR dlr = *((const DLR*)&dlrp);
    if (!dl->horizontal_shift) {
        for(size_t line = 0; line < dl->entryes->size; line++) {

            int x = dl->x;
            int y = dl->y + line;
    
            const DLE* dle = (const DLE*)vector_at(dl->entryes, line);
    
            if(dle->prop.centered) {
                x = getmaxx(win) / 2 - strnlen((const char*)vector_at(dl->entryes, line), MAX_READ) / 2;
            }
    
            if (dl->selectable && line == dl->selected && !dlr.hide_selection) {
                wattron(win, COLOR_PAIR(SELECTED_TEXT_COLOR) | A_REVERSE);
                mvwprintw(win, y, x, "%s", dle->body);
                wattroff(win, COLOR_PAIR(SELECTED_TEXT_COLOR) | A_REVERSE);
            } else {
                mvwprintw(win, y, x, "%s", dle->body);
            }
        }
    } else {
        for(size_t col = 0; col < dl->entryes->size; col++) {

            int x = dl->x + col * dl->horizontal_shift;
            int y = dl->y;
    
            const DLE* dle = (const DLE*)vector_at(dl->entryes, col);
    
            if(dle->prop.centered) {
                x = getmaxx(win) / 2 - strnlen((const char*)vector_at(dl->entryes, col), MAX_READ) / 2;
            }
    
            if (dl->selectable && col == dl->selected && !dlr.hide_selection) {
                wattron(win, COLOR_PAIR(SELECTED_TEXT_COLOR) | A_REVERSE);
                mvwprintw(win, y, x, "%s", dle->body);
                wattroff(win, COLOR_PAIR(SELECTED_TEXT_COLOR) | A_REVERSE);
            } else {
                mvwprintw(win, y, x, "%s", dle->body);
            }
        }
    }

}

char* dl_get_selected(DispayList* dl) {
    if(!dl->selectable) {
        log_message("DL not selectable");
        return NULL;
    }
    if(!dl->entryes->size) {
        return NULL;
    }
    DLE* dle = (DLE*)vector_at(dl->entryes, dl->selected);
    return dle->body;
}

void dl_free(DispayList* dl) {
    vector_free(dl->entryes);
}

void dl_set_pos(DispayList* dl, int x, int y) {
    dl->x = x;
    dl->y = y;
}

int string_compare_dle(const void *a, const void *b) {
    const DLE* f = (DLE*)a;
    const DLE* s = (DLE*)b;
    return str_compare_fun(f->body, s->body);
}

int natural_compare_dle(const void *a, const void *b) {
    const DLE *first = (const DLE*)a;
    const DLE *second = (const DLE*)b;
    const char *s1 = first->body;
    const char *s2 = second->body;

    
    while (*s1 && *s2) {
        if (isdigit(*s1) && isdigit(*s2)) {
            long num1 = strtol(s1, (char**)&s1, 10);
            long num2 = strtol(s2, (char**)&s2, 10);
            
            if (num1 != num2)
                return (num1 > num2) - (num1 < num2);
        } else {

            if (*s1 != *s2)
                return (*s1 > *s2) - (*s1 < *s2);
            s1++;
            s2++;
        }
    }
    return (*s1 != '\0') - (*s2 != '\0');
}

void dl_sort_natural(DispayList* dl) {
    vector_sort(dl->entryes, natural_compare_dle);
}

void dl_reset_sel_pos(DispayList* dl) {
    if (!dl->selectable)
        dl->selectable = TRUE;

    if(!dl->entryes->size) {
        return;
    }

    if(dl->selected >= dl->entryes->size) {
        dl->selected = dl->entryes->size - 1;
    }

    DLE* dle = (DLE*)vector_at(dl->entryes, dl->selected);
    if (!dle->prop.unselectable) {
        return;
    }

    if(!dl_iterate(dl, +1)) {
        dl->selectable = false;
    }
}