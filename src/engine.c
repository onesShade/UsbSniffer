#define _POSIX_C_SOURCE 200809L

#include <linux/limits.h>
#include <string.h>

#include "../include/engine.h"
#include "../include/globals.h"
#include "../include/util.h"
#include "../include/storageTest.h"
#include "../include/fileSystem.h"

void update_bottom_line_dl() {
    dl_clear(bottom_line_dl);

    if (selection.window != storage_test_results) {
        dl_add_entry(bottom_line_dl, DLEP_NONE, "F10 - EXIT");
    } else {
        dl_add_entry(bottom_line_dl, DLEP_NONE, "Q - BACK");
    }

    if(selection.window == device_list) {
        if (is_storage_device(selection.device_name) && devices_dl->entryes->size) {
            dl_add_entry(bottom_line_dl, DLEP_NONE, "F2 - TEST");
        }
    }   

    if(selection.window == storage_test_settings) {
        dl_add_entry(bottom_line_dl, DLEP_NONE, "Q - BACK");
        dl_add_entry(bottom_line_dl, DLEP_NONE, "V - TEST");
        dl_add_entry(bottom_line_dl, DLEP_NONE, "M - MOUNT P.");
        dl_add_entry(bottom_line_dl, DLEP_NONE, "B - FILE SIZE");
        dl_add_entry(bottom_line_dl, DLEP_NONE, "N - PASSES");
         if (strncmp("RR", dl_get_selected(test_mode_sel_dl), MAX_READ) == 0) {
            dl_add_entry(bottom_line_dl, DLEP_NONE, "L - BLOCK");
        }
    }
}

void update_keys(int key) { 
    update_bottom_line_dl();
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
    
    switch (selection.window) {
        case device_list: {
            if (key == KEY_DOWN) {
                dl_iterate(devices_dl, +1);   
                update_cycle_counter = 0;
                update_bottom_line_dl();
            }
            if (key == KEY_UP) {
                dl_iterate(devices_dl, -1);
                update_cycle_counter = 0;
                update_bottom_line_dl();
            }
            
            if (key == 'q') {
                is_open = FALSE;  
            }

            if (key == KEY_F(2) && is_storage_device(selection.device_name)) {
                set_current_window(storage_test_settings);
                set_test_props();
            }
        } break;
        case storage_test_settings: {
            update_st_test_settings(key);
        } break;
        case storage_test_results: {
            if (key == 'q') {
                set_current_window(device_list);
            }
        } break;
        case storage_test_run: 
            break;
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
        s_strcpy(selection.device_name, dl_get_selected(devices_dl), MAX_DL_STR);
    }
}

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

void update_mount_points() {
    dl_clear(mount_point_dl);
    if (!is_storage_device(selection.device_name))
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
            strstr(buffer, selection.block_name) != NULL) {
                s_strcpy(mount_buffers[mount_count], buffer, PATH_MAX);
                mount_count++;
        }
    }
    if (mount_count)
        qsort(mount_buffers, mount_count, sizeof(mount_buffers[0]), str_compare_second_substr_fun);

    for (size_t mp = 0; mp < mount_count; mp++) {
        char block_path[MAX_READ];
        char path_mount[PATH_MAX];
        char file_system[MAX_READ];
        
        sscanf(mount_buffers[mp], "%s%s%s", block_path, path_mount, file_system);
        use_octal_escapes(path_mount);
        
        extract_top_dir(block_path, buffer);

        snprintf(buffer_extra, sizeof(buffer_extra), "%s/%s/size", selection.block_path, buffer);
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

    snprintf(path_temp, sizeof(path_temp), "%s%s", SYSFS_USB_DEVICES, selection.device_name);
    if (!traverse_path(path_temp, traverce_arg, path_block)) {
        return 0;
    }
    
    extract_top_dir(path_block, path_temp);
    snprintf(selection.block_name, PATH_MAX, "/%s", path_temp);
    dl_add_entry(atr_dl,DLEP_NONE,  "Block path is :");
    dl_add_entry(atr_dl,DLEP_NONE,  "%s", path_block);
    s_strcpy(selection.block_path, path_block, PATH_MAX);
    
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
    dl_add_entry(atr_dl, DLEP_NONE,"Device name: %s", selection.device_name);
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s%s/", SYSFS_USB_DEVICES, selection.device_name);
    char count = 0;
    int atr_c = 0;
    for (; args[count].attribute_name ; count++) {
        atr_c += print_attribute_value(path, args[count], atr_dl);
    }

    if (is_storage_device(selection.device_name) && atr_c > 3) {
        print_storage_device_info();
        dl_add_entry(atr_dl, DLEP_NONE, "Block name: %s", selection.block_name);
        update_mount_points(); 
    }
}

void update_sel_dls() {
    const char* passes_n[] = {"0001", "0003", "0005", "0010", "0025", "0100","0500",NULL};
    const char* passes_n_rand[] = {"0100", "0250", "0500", "1000", "2000", "5000","10000",NULL};

    dl_clear(test_passes_sel_dl);
    dl_clear(test_passes_sel_dl);

    dl_add_entry(test_passes_sel_dl, DLEP_UNSELECTABLE, "passes: "); 
    if (strncmp("RR", dl_get_selected(test_mode_sel_dl), MAX_READ) == 0) {
        for (int i = 0; passes_n_rand[i]; i++) {
            dl_add_entry(test_passes_sel_dl, DLEP_NONE, passes_n_rand[i]);
        }
        test_block_size_sel_dl->dlp.invisible = false;
    } else {
        for (int i = 0; passes_n[i]; i++) {
            dl_add_entry(test_passes_sel_dl, DLEP_NONE, passes_n[i]);
        }
        test_block_size_sel_dl->dlp.invisible = true;
    }
    dl_add_entry(test_passes_sel_dl, DLEP_UNSELECTABLE, "TIMES");
    dl_reset_sel_pos(test_passes_sel_dl);
}

void set_current_window(Window win) {
    selection.window = win;
    update_bottom_line_dl();
}

void update_all_windows() {
    update_cycle_counter--;
    if(update_cycle_counter <= 0) {
        update_devices();
        update_attributes();
        update_cycle_counter = CICLES_TO_UPDATE;
    }
}