#include <linux/limits.h>
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include <ncurses.h>
#include <execinfo.h>
#include "storageTest.h"
#include "util.h"
#include "defines.h"
#include "globals.h"
#include "fileSystem.h"

void print_attribute_value(const char* dir,const Atr_Print_arg arg, int y, WINDOW *win) {
    char buffer[MAX_READ];
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s", dir, arg.attribute_name);
    read_usb_attribute(path, buffer, sizeof(buffer));
    if(!buffer[0]) {
        return;
    }
    mvwprintw(win, y, 1, "%s: %s %s\n",
         arg.print_prefix, buffer, arg.print_postfix ? arg.print_postfix : "");
}

int print_storage_device_info() {
    char path_temp[PATH_MAX];
    char path_block[PATH_MAX];
    char buffer[MAX_READ];

    il_info.curr_y++;
    mvwprintw(right_win, il_info.curr_y++, COLS / 4 - 17 / 2 , "-STORAGE DEVICE-");

    FindEntryArg traverce_arg[] = {
        {filter_has_simbol, (const void*)":"},
        {filter_prefix, (const void*)"host"},
        {filter_prefix, (const void*)"target"},
        {filter_has_simbol, (const void*)":"},
        {filter_prefix, (const void*)"block"},
        {filter_regular_entries, NULL},
        {NULL, NULL}
    };

    snprintf(path_temp, sizeof(path_temp), "%s%s", SYSFS_USB_DEVICES, cursor.device_name);
    if (!traverse_path(path_temp, traverce_arg, path_block)) {
        return 0;
    }

    extract_top_dir(path_block, path_temp);
    snprintf(cursor.block_name, PATH_MAX, "/%s", path_temp);

    mvwprintw(right_win, il_info.curr_y++, 1, "Block path is :");
    mvwprintw(right_win, il_info.curr_y++, 1, "%s", path_block);
    
    snprintf(path_temp, sizeof(path_temp), "%s/size", path_block);
    read_usb_attribute(path_temp, buffer, sizeof(buffer));
    unsigned long int size = 0;
    sscanf(buffer, "%ld", &size);
    mvwprintw(right_win, il_info.curr_y++, 1, "Size: %ld MB\n", size * 512 / 1024 / 1024);

    snprintf(path_temp, sizeof(path_temp), "%s%s/", SYSFS_USB_DEVICES, cursor.device_name);
    print_attribute_value(path_temp, (const Atr_Print_arg){"bMaxPower", "Max Power: ", NULL}, il_info.curr_y++, right_win);
    return 1;
}

void get_mount_points() {
    char buffer[MAX_READ];

    FILE* mounts = fopen(PROC_MOUNTS, "r");
    if(!mounts) {
        return;
    }
    il_info.curr_y++;

    char mounted = 0;
    while (fgets(buffer, sizeof(buffer), mounts)) {
        if ((strcspn(buffer, "\n") < strnlen(buffer, MAX_READ)))
            buffer[strcspn(buffer, "\n")] = 0;

        if (strstr(buffer, cursor.block_name) != NULL) {
            char path_name[MAX_READ];
            char path_mount[MAX_READ];
            char file_system[MAX_READ];

            sscanf(buffer, "%s%s%s", &path_name, &path_mount, &file_system);
            use_octal_escapes(path_mount);
            mvwprintw(right_win, il_info.curr_y++, 1, "Filesytem: %s", file_system);
            mvwprintw(right_win, il_info.curr_y++, 1, "Mount point:");
            mvwprintw(right_win, il_info.curr_y++, 1, "%s", path_mount);
            mounted = 1;
            strcpy(cursor.mount_path, path_mount);
        }
    }
    if (!mounted) {
        mvwprintw(right_win, il_info.curr_y++, 1, "[none]");
    }
    fclose(mounts);
}

void draw_right_window() {
    werase(right_win);
    il_info.curr_y = WIDOW_TOP_PADDING;

    const Atr_Print_arg args[] = {
        {"idVendor", "Vendor ID", NULL},
        {"idProduct", "Product ID", NULL},
        {"bcdDevice", "Device Version", NULL},
        {"bDeviceClass", "Device Class", NULL},
        {"bDeviceSubClass", "Device Subclass", NULL},
        {"bDeviceProtocol", "Device Protocol", NULL},
        {"manufacturer", "Manufacturer", NULL},
        {"product", "Product", NULL},
        {"serial", "Serial", NULL},
        {"version", "USB Version", NULL},
        {"speed", "Speed", "Mbps"},
        {"busnum", "Bus Number", NULL},
        {"devnum", "Device Number", NULL},
        {"maxchild", "Max Children", NULL},
        {NULL, NULL, NULL}
    };

    mvwprintw(right_win, il_info.curr_y++, 1, "Device name: %s", cursor.device_name);
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s/", SYSFS_USB_DEVICES, cursor.device_name);
    char count = 0;
    for (; args[count].attribute_name ; count++) {
        print_attribute_value(path, args[count], il_info.curr_y++, right_win);
    }

    if (is_storage_device(cursor.device_name)) {
        print_storage_device_info();
        mvwprintw(right_win, il_info.curr_y++, 1, "Block name: %s", cursor.block_name);
        get_mount_points();
    }
    box(right_win, 0, 0);
    wrefresh(right_win);
}

void draw_left_window() {
    werase(left_win);
    box(left_win, 0, 0);

    DIR *dir;
    struct dirent *ent;

    int y = 2;
    int i = 0;
    if ((dir = opendir(SYSFS_USB_DEVICES)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            
            if (!is_usb_device(ent->d_name)) {
                continue;
            }
            i++;
            if(cursor.window == device_list && y == cursor.y) {
                wattron(left_win, COLOR_PAIR(SELECTED_TEXT_COLOR) | A_REVERSE);
                mvwprintw(left_win, y++, 1, "%s", ent->d_name);
                wattroff(left_win, COLOR_PAIR(SELECTED_TEXT_COLOR) | A_REVERSE);
                strcpy(cursor.device_name, ent->d_name);
            } else {
                mvwprintw(left_win, y++, 1, "%s", ent->d_name);
            }
        }
        closedir(dir);
    } else {
        log_message("Could not open USB devices directory");
    }
    wrefresh(left_win);

    if (dl_info.len && i > dl_info.len) {
        msleep(HOTPLUG_SLEEP_MS);
    }
    dl_info.len = i;
}

void draw_bottom_window() {
    werase(bottom_win);
    int x = -16;
    
    if (is_storage_device(cursor.device_name)) {
        mvwprintw(bottom_win, 0, x += 16, "F2 - TEST");
    }
    mvwprintw(bottom_win, 0, x += 16, "F10 - EXIT");
    wrefresh(bottom_win);
}

void update(int key) {
    if (key == KEY_RESIZE) {
        endwin();
        refresh();
        clear();
        resize_term(0, 0);
        reinit_windows();
    }
    if (key == KEY_DOWN && cursor.y <= dl_info.len + WIDOW_TOP_PADDING) {
        if (cursor.y == dl_info.len + WIDOW_TOP_PADDING - 1) 
            cursor.y = WIDOW_TOP_PADDING;
        else 
            cursor.y++;
        cursor.mount_path[0] = 0;
    }
    if (key == KEY_UP && cursor.y >= WIDOW_TOP_PADDING) {
        if(cursor.y == WIDOW_TOP_PADDING)
            cursor.y = dl_info.len + WIDOW_TOP_PADDING - 1;
        else
            cursor.y--;
        cursor.mount_path[0] = 0;
    }

    if(key == 266 && is_storage_device(cursor.device_name)) {   //F2
        test_storage(cursor.mount_path, 100);
    }

    if(cursor.y > dl_info.len + WIDOW_TOP_PADDING - 1 && cursor.y > WIDOW_TOP_PADDING) {
        cursor.y--;
    }
}

int main() {
    init_log();
    init_ncurses();
    init_globals();


    cursor.window = device_list;
    cursor.y = WIDOW_TOP_PADDING;
    cursor.mount_path[0] = 0;
    while (1) {
        int key = getch();
        if (key == 'q' || key == 274) break;  //F10
        update(key);

        refresh();
        clear();
        
        draw_left_window();
        draw_right_window();
        draw_bottom_window();

        msleep(MAIN_LOOP_SLEEP_TIME_MS);
    }
    free_globals();
    return 0;
}