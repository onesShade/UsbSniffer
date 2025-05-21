#include <ncurses.h>

#include "../include/renderer.h"
#include "../include/dispayList.h"
#include "../include/globals.h"
#include "../include/util.h"
#include "../include/storageTest.h"


void draw_left_window();
void draw_right_window();
void draw_bottom_window();
void draw_popup_window();

void draw_all_windows() {
    refresh();
    clear();

    draw_left_window();
    draw_right_window();
    draw_bottom_window();
    draw_popup_window();
}

void draw_left_window() {
    werase(left_win);
    box(left_win, 0, 0);
    dl_draw(devices_dl, left_win, DLRP_NONE);
    wrefresh(left_win);
}

void draw_right_window() {
    werase(right_win);
    dl_draw(atr_dl, right_win, DLRP_NONE);
    if (is_storage_device(selection.device_name)) {
        dl_set_pos(mount_point_dl, 1,  atr_dl->entryes->size + atr_dl->y + 2);
        dl_draw(mount_point_dl, right_win, DLRP_HIDE_SELECTION);
    }
    box(right_win, 0, 0);
    wrefresh(right_win);
}

void draw_bottom_window() {
    werase(bottom_win);
    dl_draw(bottom_line_dl, bottom_win, DLRP_NONE);
    wrefresh(bottom_win);
}

void draw_popup_window() {
    if(selection.window == device_list)
        return;

    werase(popup_win);

    switch (selection.window) {
        case storage_test_settings: {
            if (is_storage_device(selection.device_name)) {
                dl_set_pos(mount_point_dl, 4, 1);
                dl_draw(mount_point_dl, popup_win, DLRP_NONE);
            }
        
            int y_p = mount_point_dl->y + mount_point_dl->entryes->size;

            mvwprintw_centered(popup_win, y_p + 1, "---TEST SETTINGS---");
            
            dl_set_pos(test_mode_sel_dl, test_mode_sel_dl->x, y_p + 3);
            dl_draw(test_mode_sel_dl, popup_win, DLRP_NONE);
        
            dl_set_pos(test_size_sel_dl, test_passes_sel_dl->x, y_p + 5);
            dl_draw(test_size_sel_dl, popup_win, DLRP_NONE);
        
            dl_set_pos(test_passes_sel_dl, test_passes_sel_dl->x, y_p + 7);
            dl_draw(test_passes_sel_dl, popup_win, DLRP_NONE);

            dl_set_pos(test_block_size_sel_dl, test_block_size_sel_dl->x, y_p + 9);
            dl_draw(test_block_size_sel_dl, popup_win, DLRP_NONE);

            mvwprintw(popup_win, test_passes_sel_dl->y + 6, test_passes_sel_dl->x, "%s", testPropsStr);
        } break;
        case storage_test_results:
        case storage_test_run: {
            dl_draw(test_screen_dl, popup_win, DLRP_NONE);
        } break;
        default:
            break;
    }
   
    box(popup_win, 0, 0);
    wrefresh(popup_win);
}