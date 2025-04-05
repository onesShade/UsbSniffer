#include "dispayList.h"
#include "util.h"
#include "vector.h"
#include <linux/limits.h>
#include <ncurses.h>
#include <stdlib.h>
#include "defines.h"
#include <string.h>

DispayList* dl_init(char selectable, int x, int y) {
    DispayList* dl = malloc(sizeof(DispayList));
    dl->selectable = selectable;
    dl->selected = 0;
    dl->x = x;
    dl->y = y;
    dl->entryes = vector_init(MAX_DL_STR, 16);
    return dl;
}

void dl_iterate(DispayList* dl, int move) {
    if((move != 1 && move != -1) || !dl->selectable) {
        log_message("dl_iterate exception");
        return;
    }
    dl->selected += move;
    dl->selected += dl->entryes->size;
    dl->selected = dl->selected % dl->entryes->size;
}

void dl_clear(DispayList* dl) {
    vector_clear(dl->entryes);
}

void dl_add_entry(DispayList* dl, const char *format, ...) {
    char buff[PATH_MAX];
    int size = 0;

    va_list args;
    va_start(args, format);
    size = vsprintf(buff, format, args);
    va_end(args);

    if (size >= MAX_DL_STR) {
        log_message("DL entry size overflow on %s", buff);
        return;
    }
    vector_push_back(dl->entryes, buff);
}

void dl_draw(DispayList* dl, WINDOW* win) {
    while (dl->selectable && dl->selected >= dl->entryes->size && dl->entryes->size) 
        dl->selected--;

    for(size_t line = 0; line < dl->entryes->size; line++) {
        if(((char*)(vector_at(dl->entryes, line)))[0] == '@') {
            int x = getmaxx(win) / 2 - strlen((const char*)vector_at(dl->entryes, line)) / 2;
            mvwprintw(win, dl->y + line, x, "%s", 
                (char*)(vector_at(dl->entryes, line) + 1));
            continue;
        }

        if (dl->selectable && line == dl->selected) {
            wattron(win, COLOR_PAIR(SELECTED_TEXT_COLOR) | A_REVERSE);
            mvwprintw(win, dl->y + line, dl->x, "%s", 
                (char*)vector_at(dl->entryes, line));
            wattroff(win, COLOR_PAIR(SELECTED_TEXT_COLOR) | A_REVERSE);
        } else {
            mvwprintw(win, dl->y + line, dl->x, "%s", 
                (char*)vector_at(dl->entryes, line));
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