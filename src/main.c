#include <stddef.h>
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

int print_attribute_value(const char* dir,const Atr_Print_arg arg, DispayList *dl) {
    char buffer[MAX_READ];
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s", dir, arg.attribute_name);
    read_usb_attribute(path, buffer, sizeof(buffer));
    if(!buffer[0]) {
        dl_add_entry(dl, DLEP_NONE,"%s: ---", arg.print_prefix);
        return 0;
    }
    dl_add_entry(dl, DLEP_NONE,"%s: %s %s",
         arg.print_prefix, buffer, arg.print_postfix ? arg.print_postfix : "");
    return 1;
}

int print_storage_device_info() {
    char path_temp[PATH_MAX];
    char path_block[PATH_MAX];
    char buffer[MAX_READ];

    dl_add_entry(atr_dl, DLEP_CENTERED, "---STORAGE DEVICE---");

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
    dl_add_entry(atr_dl,DLEP_NONE,  "Block path is :");
    dl_add_entry(atr_dl,DLEP_NONE,  "%s", path_block);
    s_strcpy(selection_lw.block_path, path_block, PATH_MAX);
    
    snprintf(path_temp, sizeof(path_temp), "%s/size", path_block);
    read_usb_attribute(path_temp, buffer, sizeof(buffer));
    unsigned long int size = 0;
    sscanf(buffer, "%ld", &size);
    dl_add_entry(atr_dl,DLEP_NONE, "Size: %ld MB\n", size * 512 / 1024 / 1024);

    snprintf(path_temp, sizeof(path_temp), "%s%s/", SYSFS_USB_DEVICES, selection_lw.device_name);
    print_attribute_value(path_temp, (const Atr_Print_arg){"bMaxPower", "Max Power: ", NULL},  atr_dl);
    return 1;
}

void get_mount_points() {
    dl_clear(mount_point_dl);
    if (!is_storage_device(selection_lw.device_name))
        return;

    char buffer[PATH_MAX];
    char buffer_extra[PATH_MAX];
    char mount_buffers[MAX_MOUNT_POINTS][PATH_MAX];
    FILE* mounts = fopen(PROC_MOUNTS, "r");
    if(!mounts) {
        return;
    }

    dl_add_entry(mount_point_dl, DLEP_CENTERED | DLEP_UNSELECTABLE, "---MOUNT POINTS---");

    size_t mount_count = 0;
    while (fgets(buffer, sizeof(buffer), mounts) && mount_count < MAX_MOUNT_POINTS) {
        if (strcspn(buffer, "\n") < strnlen(buffer, MAX_READ))
            buffer[strcspn(buffer, "\n")] = 0;

        if (strstr(buffer, selection_lw.block_name) != NULL) {
            s_strcpy(mount_buffers[mount_count], buffer, PATH_MAX);
            mount_count++;
        }
    }
    if(mount_count)
        qsort(mount_buffers, mount_count, sizeof(mount_buffers[0]), str_compare_second_substr_fun);

    for(size_t mp = 0; mp < mount_count; mp++) {
        char block_path[MAX_READ];
        char path_mount[PATH_MAX];
        char file_system[MAX_READ];
        
        sscanf(mount_buffers[mp], "%s%s%s", block_path, path_mount, file_system);
        use_octal_escapes(path_mount);
        
        extract_top_dir(block_path, buffer);

        snprintf(buffer_extra, sizeof(buffer_extra), "%s/%s/size", selection_lw.block_path, buffer);
        read_usb_attribute(buffer_extra, buffer, sizeof(buffer));
        unsigned long int size = 0;
        sscanf(buffer, "%ld", &size);
        
        dl_add_entry(mount_point_dl, DLEP_UNSELECTABLE, "%d. FS: %s\t\tSIZE: %ld MB", mp + 1, file_system, size * 512 / 1024 / 1024);
        dl_add_entry(mount_point_dl, DLEP_NONE,"%s", path_mount);

        s_strcpy(selection_lw.mount_path, path_mount, PATH_MAX);
    }

    if (!mount_count) {
        dl_add_entry(mount_point_dl, DLEP_NONE, "[none]");
    }
    fclose(mounts);
}

void draw_right_window() {
    werase(right_win);
    dl_draw(atr_dl, right_win, DLRP_NONE);
    if (is_storage_device(selection_lw.device_name)) {
        dl_set_pos(mount_point_dl, 1,  atr_dl->entryes->size + atr_dl->y + 2);
        dl_draw(mount_point_dl, right_win, DLRP_HIDE_SELECTION);
    }
    box(right_win, 0, 0);
    wrefresh(right_win);
}

void draw_left_window() {
    werase(left_win);
    box(left_win, 0, 0);
    //update_device_dl();
    dl_draw(devices_dl, left_win, DLRP_NONE);
    wrefresh(left_win);
}

void draw_bottom_window() {
    werase(bottom_win);
    int x = -16;
    if(selection_lw.window == device_list) {
        if (is_storage_device(selection_lw.device_name)) {
            mvwprintw(bottom_win, 0, x += 16, "F2 - TEST");
        }
    }
    if(selection_lw.window == storage_test_settings) {
        mvwprintw(bottom_win, 0, x += 16, "M - MOUNT P.");
        mvwprintw(bottom_win, 0, x += 16, "B - FILE SIZE");
        mvwprintw(bottom_win, 0, x += 16, "N - PASSES");
    }
    mvwprintw(bottom_win, 0, x += 16, "F10 - EXIT");
    wrefresh(bottom_win);
}

void draw_popup_window() {
    werase(popup_win);
    if (is_storage_device(selection_lw.device_name)) {
        dl_set_pos(mount_point_dl, 4, 1);
        dl_draw(mount_point_dl, popup_win, DLRP_NONE);
    }
    dl_set_pos(test_size_sel_dl, test_size_sel_dl->x, mount_point_dl->y + mount_point_dl->entryes->size + 1);
    dl_draw(test_size_sel_dl, popup_win, DLRP_NONE);

    dl_set_pos(test_passes_dl, test_passes_dl->x, test_size_sel_dl->y + 2);
    dl_draw(test_passes_dl, popup_win, DLRP_NONE);

    mvwprintw(popup_win, test_passes_dl->y + 4, 1, "%s", testPropsStr);

    box(popup_win, 0, 0);
    wrefresh(popup_win);
}

void update_keys(int key) { 
    if (key == KEY_F(10)) {
        is_open = FALSE;
    }

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
        
        if (key == 'q') {
            is_open = FALSE;  
        }

        if(key == KEY_F(2) && is_storage_device(selection_lw.device_name)) {
            selection_lw.window = storage_test_settings;
            set_test_props();
        }
    }
    if (selection_lw.window == storage_test_settings) {
        update_st_test_settings(key);
    }

    if(dl_get_selected(devices_dl)) {
        s_strcpy(selection_lw.device_name, dl_get_selected(devices_dl), MAX_DL_STR);
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
            dl_add_entry(devices_dl, DLEP_NONE,"%s", ent->d_name);
        }
        closedir(dir);
    } else {
        log_message("Could not open USB devices directory");
    }

    dl_sort_natural(devices_dl);
    //dl_draw(devices_dl, left_win, DLRP_NONE);
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
    dl_add_entry(atr_dl, DLEP_CENTERED,"---USB DEVICE ATTRIBUTES---");
    dl_add_entry(atr_dl, DLEP_NONE,"Device name: %s", selection_lw.device_name);
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s/", SYSFS_USB_DEVICES, selection_lw.device_name);
    char count = 0;
    int atr_c = 0;
    for (; args[count].attribute_name ; count++) {
        atr_c += print_attribute_value(path, args[count], atr_dl);
    }

    if (is_storage_device(selection_lw.device_name) && atr_c > 3) {
        print_storage_device_info();
        dl_add_entry(atr_dl, DLEP_NONE, "Block name: %s", selection_lw.block_name);
        get_mount_points(); 
    }
    box(right_win, 0, 0);

}

int main() {
    init_log();
    init_ncurses();
    init_globals();

    selection_lw.window = device_list;
    selection_lw.mount_path[0] = 0;
    while (is_open) {
        int key = getch();
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