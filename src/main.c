#include "../include/util.h"
#include "../include/globals.h"
#include "../include/renderer.h"
#include "../include/engine.h"

int main() {
    init_log();
    init_ncurses();
    init_globals();

    set_current_window(device_list);
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