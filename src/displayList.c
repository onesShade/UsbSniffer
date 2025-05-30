#define _POSIX_C_SOURCE 199309L 
#define _GNU_SOURCE

#include <stddef.h>
#include <linux/limits.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/defines.h"
#include "../include/dispayList.h"
#include "../include/util.h"
#include "../include/vector.h"

DispayList* dl_init(DLPe dlp, const int x, const int y) {
    DispayList* dl = malloc(sizeof(DispayList));

    dl->dlp.selectable = (dlp & DLP_SELECTABLE) ? 1 : 0;
    dl->dlp.invisible  = (dlp & DLP_INVISIBLE)  ? 1 : 0;
    dl->dlp.horizontal = (dlp & DLP_HORIZONTAL) ? 1 : 0;

    dl->selected = 0;
    dl->x = x;
    dl->y = y;

    if (dl->dlp.horizontal) {
        dl->horizontal_shift = DEFAULT_HORIZONTAL_SHIFT;
    } else {
        dl->horizontal_shift = 0;
    }

    dl->entryes = vector_init(sizeof(DLE), 16);
    return dl;
}

int dl_iterate(DispayList* dl, int move) {
    if(!dl->entryes->size || dl->dlp.invisible) {
        return 0;
    }

    if((move != 1 && move != -1) || !dl->dlp.selectable) {
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

DLE* dl_add_entry(DispayList* dl, DLEPe dlep, const char *format, ...) {
    char buff[PATH_MAX];
    int size = 0;
    va_list args;
    va_start(args, format);
    size = vsprintf(buff, format, args);
    va_end(args);
    buff[MAX_DL_STR] = dlep;
    if (size >= MAX_DL_STR) {
        log_message("DL entry size overflow on %s", buff + 1);
        return NULL;
    }
    vector_push_back(dl->entryes, buff);
    return (DLE*)vector_at(dl->entryes, dl->entryes->size - 1);
}

void dl_draw(const DispayList* dl, WINDOW* win, const DLRPe dlrp) {
    if (dl->dlp.invisible) {
        return;
    }

    const DLRP dlr = *((const DLRP*)&dlrp);
    if (!dl->dlp.horizontal) {
        for(size_t line = 0; line < dl->entryes->size; line++) {

            int x = dl->x;
            int y = dl->y + line;
    
            const DLE* dle = (const DLE*)vector_at(dl->entryes, line);
    
            if(dle->prop.centered) {
                x = getmaxx(win) / 2 - strnlen((const char*)vector_at(dl->entryes, line), MAX_READ) / 2;
            }
    
            if (dl->dlp.selectable && line == dl->selected && !dlr.hide_selection) {
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
    
            if (dl->dlp.selectable && col == dl->selected && !dlr.hide_selection) {
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
    if(!dl->dlp.selectable) {   
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
    const DLE* f = (const DLE*)a;
    const DLE* s = (const DLE*)b;
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
    if (!dl->dlp.selectable)
        dl->dlp.selectable = true;

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
        dl->dlp.selectable = false;
    }
}