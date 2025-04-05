#define _POSIX_C_SOURCE 200809L
#include "dispayList.h"

#include <linux/limits.h>
#include "fileSystem.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
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

void update_atr_dl();
void update_device_dl();

void print_attribute_value(const char* dir,const Atr_Print_arg arg, WINDOW *win) {
    char buffer[MAX_READ];
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s", dir, arg.attribute_name);
    read_usb_attribute(path, buffer, sizeof(buffer));
    if(!buffer[0]) {
        dl_add_entry(atr_dl, "%s: ---", arg.print_prefix);
        return;
    }
    dl_add_entry(atr_dl, "%s: %s %s",
         arg.print_prefix, buffer, arg.print_postfix ? arg.print_postfix : "");
}

int print_storage_device_info() {
    char path_temp[PATH_MAX];
    char path_block[PATH_MAX];
    char buffer[MAX_READ];

    dl_add_entry(atr_dl, "@---STORAGE DEVICE---");

    FindEntryArg traverce_arg[] = {
        {filter_has_simbol, (const void*)":"},
        {filter_prefix, (const void*)"host"},
        {filter_prefix, (const void*)"target"},
        {filter_has_simbol, (const void*)":"},
        {filter_prefix, (const void*)"block"},
        {filter_regular_entries, NULL},
        {NULL, NULL}
    };

    snprintf(path_temp, sizeof(path_temp), "%s%s", SYSFS_USB_DEVICES, selection_lw.device_name);
    if (!traverse_path(path_temp, traverce_arg, path_block)) {
        return 0;
    }

    extract_top_dir(path_block, path_temp);
    snprintf(selection_lw.block_name, PATH_MAX, "/%s", path_temp);
    dl_add_entry(atr_dl,  "Block path is :");
    dl_add_entry(atr_dl,  "%s", path_block);

    
    snprintf(path_temp, sizeof(path_temp), "%s/size", path_block);
    read_usb_attribute(path_temp, buffer, sizeof(buffer));
    unsigned long int size = 0;
    sscanf(buffer, "%ld", &size);
    dl_add_entry(atr_dl, "Size: %ld MB\n", size * 512 / 1024 / 1024);

    snprintf(path_temp, sizeof(path_temp), "%s%s/", SYSFS_USB_DEVICES, selection_lw.device_name);
    print_attribute_value(path_temp, (const Atr_Print_arg){"bMaxPower", "Max Power: ", NULL},  right_win);
    return 1;
}

void get_mount_points() {
    dl_clear(mount_point_dl);
    if (!is_storage_device(selection_lw.device_name))
        return;

    mount_point_dl->y = atr_dl->entryes->size + atr_dl->y + 2;
    char buffer[MAX_READ];

    FILE* mounts = fopen(PROC_MOUNTS, "r");
    if(!mounts) {
        return;
    }

    char mounted = 0;
    while (fgets(buffer, sizeof(buffer), mounts)) {
        if ((strcspn(buffer, "\n") < strnlen(buffer, MAX_READ)))
            buffer[strcspn(buffer, "\n")] = 0;

        if (strstr(buffer, selection_lw.block_name) != NULL) {
            char path_name[MAX_READ];
            char path_mount[PATH_MAX];
            char file_system[MAX_READ];
            
            sscanf(buffer, "%s%s%s", path_name, path_mount, file_system);
            use_octal_escapes(path_mount);

            dl_add_entry(mount_point_dl, "Filesytem: %s", file_system);
            dl_add_entry(mount_point_dl, "%s", path_mount);

            mounted = 1;
            strcpy(selection_lw.mount_path, path_mount);
        }
    }
    if (!mounted) {
        dl_add_entry(mount_point_dl, "[none]");
    }
    fclose(mounts);
}

void draw_right_window() {
    werase(right_win);
    dl_draw(atr_dl, right_win);
    if (is_storage_device(selection_lw.device_name)) {
        dl_draw(mount_point_dl, right_win);
        mvwprintw(right_win, mount_point_dl->y - 1, mount_point_dl->x,  "    Mount points:");
    }

    box(right_win, 0, 0);
    wrefresh(right_win);
}

void draw_left_window() {
    werase(left_win);
    box(left_win, 0, 0);
    update_device_dl();
    wrefresh(left_win);
}

void draw_bottom_window() {
    werase(bottom_win);
    int x = -16;
    
    if (is_storage_device(selection_lw.device_name)) {
        mvwprintw(bottom_win, 0, x += 16, "F2 - TEST");
    }
    mvwprintw(bottom_win, 0, x += 16, "F10 - EXIT");
    wrefresh(bottom_win);
}

void draw_popup_window() {
    werase(popup_win);
    box(popup_win, 0, 0);
    wrefresh(popup_win);
}

void update_keys(int key) {
    if (key == KEY_RESIZE) {
        endwin();
        refresh();
        clear();
        resize_term(0, 0);
        reinit_windows();
    }
    
    if (selection_lw.window == device_list) { 
        if (key == KEY_DOWN) {
            dl_iterate(devices_dl, +1);   
            update_cycle_counter = 0;
        }
        if (key == KEY_UP) {
            dl_iterate(devices_dl, -1);
            update_cycle_counter = 0;
        }

        if(key == 266 && is_storage_device(selection_lw.device_name)) {   //F2
            selection_lw.window = storage_test_settings;
            //test_storage(selection_lw.mount_path, 100);
        }
    }
    if (selection_lw.window == storage_test_settings) {
        update_st_test_settings(key);
    }

    if(dl_get_selected(devices_dl)) {
        strncpy(selection_lw.device_name, dl_get_selected(devices_dl), MAX_DL_STR);
    }
}

void update() {
    update_cycle_counter--;
    if(update_cycle_counter <= 0) {
        update_device_dl();
        update_atr_dl();
        update_cycle_counter = CICLES_TO_UPDATE;
    }
}

void update_device_dl() {
    DIR *dir;
    struct dirent *ent;
    dl_clear(devices_dl);

    if ((dir = opendir(SYSFS_USB_DEVICES)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            
            if (!is_usb_device(ent->d_name)) {
                continue;
            }
            dl_add_entry(devices_dl, "%s", ent->d_name);
        }
        closedir(dir);
    } else {
        log_message("Could not open USB devices directory");
    }

    dl_draw(devices_dl, left_win);
}

void update_atr_dl() {
    dl_clear(atr_dl);

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
    dl_add_entry(atr_dl, "@---USB DEVICE ATTRIBUTES---");
    dl_add_entry(atr_dl, "Device name: %s", selection_lw.device_name);
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s/", SYSFS_USB_DEVICES, selection_lw.device_name);
    char count = 0;
    for (; args[count].attribute_name ; count++) {
        print_attribute_value(path, args[count], right_win);
    }

    if (is_storage_device(selection_lw.device_name)) {
        print_storage_device_info();
        dl_add_entry(atr_dl, "Block name: %s", selection_lw.block_name);
        get_mount_points(); 
    }
    box(right_win, 0, 0);

}

int main() {
    init_log();
    init_ncurses();
    init_globals();

    selection_lw.window = device_list;
    selection_lw.y = WIDOW_TOP_PADDING;
    selection_lw.mount_path[0] = 0;
    while (1) {
        int key = getch();
        if (key == 274) break;  //F10

        update_keys(key);
        update();

        refresh();
        clear();
        
        draw_left_window();
        draw_right_window();
        draw_bottom_window();

        if(selection_lw.window != device_list)
            draw_popup_window();

        msleep(MAIN_LOOP_SLEEP_TIME_MS);
    }
    free_globals();
    return 0;
}