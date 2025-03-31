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
#include <errno.h>
#include <ncurses.h>
#include <string.h>

#include "storageTest.h"
#include "util.h"
#include "defines.h"
#include "globals.h"

void print_attribute_value(const Atr_Print_arg arg, int y, WINDOW *win) {
    char path[PATH_MAX];
    char buffer[MAX_READ];

    snprintf(path, sizeof(path), "%s%s/%s", SYSFS_USB_DEVICES, cursor.device_name, arg.attribute_name);
    read_usb_attribute(path, buffer, sizeof(buffer));
    if(!buffer[0]) {
        return;
    }
    mvwprintw(win, y, 1, "%s: %s %s\n",
         arg.print_prefix, buffer, arg.print_postfix ? arg.print_postfix : "");
}

int print_storage_device_info() {
    DIR *dir = NULL;
    char path[PATH_MAX];
    char path_other[PATH_MAX];
    char path_final[PATH_MAX];
    char buffer[MAX_READ];
    struct dirent *ent;

    il_info.curr_y++;
    mvwprintw(right_win, il_info.curr_y++, 3 , "-STORAGE DEVICE-");

    snprintf(path, sizeof(path), "%s%s", SYSFS_USB_DEVICES, cursor.device_name);
    
    if (!open_dir(&dir, path)) { 
        return 0;
    }
    
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, cursor.device_name, strlen(cursor.device_name)) != 0 || 
        strchr(ent->d_name, ':') == NULL) {
            continue;
        }
        break;
    }

    snprintf(path_other, sizeof(path), "%s/%s", path, ent->d_name);

    if (!open_dir(&dir, path_other)) { 
        return 0;
    }

    const char* host = "host";
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(host, ent->d_name, strlen(host)) != 0) {
            continue;
        }
        break;
    }

    snprintf(path, sizeof(path), "%s/%s", path_other, ent->d_name);

    if (!open_dir(&dir, path)) { 
        return 0;
    }

    const char* target = "target";
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(target, ent->d_name, strlen(target)) != 0) {
            continue;
        }
        break;
    }

    snprintf(path_other, sizeof(path), "%s/%s", path, ent->d_name);
    if (!open_dir(&dir, path_other)) { 
        return 0;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (strchr(ent->d_name, ':') == NULL) {
            continue;
        }
        break;
    }

    snprintf(path, sizeof(path), "%s/%s", path_other, ent->d_name);
    if (!open_dir(&dir, path)) { 
        return 0;
    }

    snprintf(path_other, sizeof(path), "%s/block", path);

    if (!open_dir(&dir, path_other)) { 
        return 0;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') {
            continue;
        }
        break;
    }

    snprintf(cursor.block_name, PATH_MAX, "/%s", ent->d_name);

    snprintf(path_final, sizeof(path), "%s/%s", path_other, ent->d_name);
    if (!open_dir(&dir, path_final)) { 
        return 0;
    }

    mvwprintw(right_win, il_info.curr_y++, 1, "Block path is :");
    mvwprintw(right_win, il_info.curr_y++, 1, "%s", path_final);
    
    snprintf(path, sizeof(path), "%s/size", path_final);
    read_usb_attribute(path, buffer, sizeof(buffer));
    unsigned long int size = 0;
    sscanf(buffer, "%ld", &size);

    mvwprintw(right_win, il_info.curr_y++, 1, "Size: %ld MB\n", size * 512 / 1024 / 1024);

    snprintf(path, sizeof(path), "%s%s/%s", SYSFS_USB_DEVICES, cursor.device_name, "bMaxPower");
    if (access(path, F_OK) == 0) {
        read_usb_attribute(path, buffer, sizeof(buffer));
        mvwprintw(right_win, il_info.curr_y++, 1, "Max Power: %s\n", buffer);
    }
    if (dir) closedir(dir);
    return 1;
}

void get_mount_points() {
    char buffer[MAX_READ];
    char path_name[MAX_READ];
    char path_mount[MAX_READ];
    
    FILE* mounts = fopen(PROC_MOUNTS, "r");
    if(!mounts) {
        return;
    }
    il_info.curr_y++;
    mvwprintw(right_win, il_info.curr_y++, 1, "Mount point:");

    char mounted = 0;
    while (fgets(buffer, sizeof(buffer), mounts)) {
        if ((strcspn(buffer, "\n") < strnlen(buffer, MAX_READ)))
            buffer[strcspn(buffer, "\n")] = 0;

        if (strstr(buffer, cursor.block_name) != NULL) {
            sscanf(buffer, "%s%s", &path_name, &path_mount);
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

    char count = 0;
    for (; args[count].attribute_name ; count++) {
        print_attribute_value(args[count], il_info.curr_y++, right_win);
    }

    if (is_storage_device(cursor.device_name)) {
        print_storage_device_info();
        mvwprintw(right_win, il_info.curr_y++, 1, "Block name: %s", cursor.block_name);
        get_mount_points(cursor.block_name);
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