#include "dispayList.h"
#include "util.h"
#include "vector.h"
#include <linux/limits.h>
#include <ncurses.h>
#include <stdlib.h>
#include "defines.h"
#include <string.h>

DispayList* dl_init(char selectable, char horizontal_shift, int x, int y) {
    DispayList* dl = malloc(sizeof(DispayList));
    dl->selectable = selectable;
    dl->selected = 0;
    dl->x = x;
    dl->y = y;
    dl->horizontal_shift = horizontal_shift;
    dl->entryes = vector_init(sizeof(DLE), 16);
    return dl;
}

void dl_iterate(DispayList* dl, int move) {
    if((move != 1 && move != -1) || !dl->selectable) {
        log_message("dl_iterate exception");
        return;
    }
    int i = 0;
    do {
    if(i++ >= 100) {
        log_message("infinite loop at dl_iterate");
        return;
    }
    dl->selected += move;
    dl->selected += dl->entryes->size;
    dl->selected = dl->selected % dl->entryes->size;
    } while(((DLE*)vector_at(dl->entryes, dl->selected))->prop.unselectable);
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

void dl_draw(DispayList* dl, WINDOW* win, DLRProperties dlrp) {
    while (dl->selectable && dl->selected >= dl->entryes->size && dl->entryes->size) 
        dl->selected--;

    DLR dlr = *((DLR*)&dlrp);
    if (!dl->horizontal_shift) {
        for(size_t line = 0; line < dl->entryes->size; line++) {

            int x = dl->x;
            int y = dl->y + line;
    
            DLE* dle = (DLE*)vector_at(dl->entryes, line);
    
            if(dle->prop.centered) {
                x = getmaxx(win) / 2 - strlen((const char*)vector_at(dl->entryes, line)) / 2;
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
    
            DLE* dle = (DLE*)vector_at(dl->entryes, col);
    
            if(dle->prop.centered) {
                x = getmaxx(win) / 2 - strlen((const char*)vector_at(dl->entryes, col)) / 2;
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
    return vector_at(dl->entryes, dl->selected);
}

void dl_free(DispayList* dl) {
    vector_free(dl->entryes);
}

void dl_set_pos(DispayList* dl, int x, int y) {
    dl->x = x;
    dl->y = y;
}