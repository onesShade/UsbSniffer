#include "util.h"
#include "globals.h"
#include "renderer.h"
#include "engine.h"

int main() {
    init_log();
    init_ncurses();
    init_globals();

    selection_lw.window = device_list;
    while (is_open) {
        int key = getch();
        update_keys(key);

        update_all_windows();
        draw_all_windows();

        msleep(MAIN_LOOP_SLEEP_TIME_MS);
    }
    free_globals();
    return 0;
}