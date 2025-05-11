#define _POSIX_C_SOURCE 200809L

#include "engine.h"
#include <ncurses.h>
#include "globals.h"
#include "util.h"
#include "storageTest.h"
#include "fileSystem.h"
#include <string.h>

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
    
    switch (selection_lw.window) {
        case device_list: {
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

            if (key == KEY_F(2) && is_storage_device(selection_lw.device_name)) {
                selection_lw.window = storage_test_settings;
                set_test_props();
            }
        } break;
        case storage_test_settings: {
            update_st_test_settings(key);
        } break;
        case storage_test_run: {
            if (key == 'q') {
                selection_lw.window = device_list;
            }
        } break;
    }
}

void update_devices() {
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
    dl_reset_sel_pos(devices_dl);

    if(dl_get_selected(devices_dl)) {
        s_strcpy(selection_lw.device_name, dl_get_selected(devices_dl), MAX_DL_STR);
    }
}


void update_mount_points() {
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
    dl_add_entry(mount_point_dl, DLEP_UNSELECTABLE, "");

    size_t mount_count = 0;
    while (fgets(buffer, sizeof(buffer), mounts) && mount_count < MAX_MOUNT_POINTS) {
        if (strcspn(buffer, "\n") < strnlen(buffer, MAX_READ) &&
            strstr(buffer, selection_lw.block_name) != NULL) {

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
        sscanf(buffer, "%lu", &size);
        
        dl_add_entry(mount_point_dl, DLEP_UNSELECTABLE, "%d. FS: %s\t\tSIZE: %ld MB", mp + 1, file_system, size * 512 / 1024 / 1024);
        dl_add_entry(mount_point_dl, DLEP_NONE,"%s", path_mount);
    }

    if (!mount_count) {
        dl_add_entry(mount_point_dl, DLEP_UNSELECTABLE, "[none]");
    }
    fclose(mounts);
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
    sscanf(buffer, "%lu", &size);
    dl_add_entry(atr_dl,DLEP_NONE, "Size: %lu MB\n", size * 512 / 1024 / 1024);
    return 1;
}

void update_attributes() {
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
        {"serial", "Serial", NULL},
        {"version", "USB Version", NULL},
        {"speed", "Speed", "Mbps"},
        {"busnum", "Bus Number", NULL},
        {"devnum", "Device Number", NULL},
        {"maxchild", "Max Children", NULL},
        {"bMaxPower", "Max Power", NULL},
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
        update_mount_points(); 
    }
    box(right_win, 0, 0);
}

void update_all_windows() {
    update_cycle_counter--;
    if(update_cycle_counter <= 0) {
        update_devices();
        update_attributes();
        update_cycle_counter = CICLES_TO_UPDATE;
    }
}