#define _POSIX_C_SOURCE 199309L 

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <ncurses.h>

#include "storageTest.h"
#include "util.h"

#define SYSFS_USB_DEVICES "/sys/bus/usb/devices/"
#define SYS_BLOCK "/sys/block/"
#define PROC_MOUNTS "/proc/mounts"
#define MAX_READ 256
#define PATH_MAX 256

#define WIDOW_TOP_PADDING 2

#define NDEBUG

#define SELECTED_COLOR 1

typedef enum {
    device_list,
}Window;

struct {
    Window window;
    char device_name[PATH_MAX];
    char block_name[PATH_MAX];
    char mount_path[PATH_MAX];
    int y;
}cursor;

struct {
    int len;
}dl_info;

struct {
    int curr_y;
}il_info;

WINDOW *left_win;
WINDOW *right_win;
WINDOW *bottom_win;

int open_dir(DIR **dir, const char *path) {
    #ifdef DEBUG
    log_message("Trying to open %s", path);
    #endif

    if (access(path, F_OK) == -1) {
        log_message("Failed to open %s\n", path);
        return 0;
    }   

    if (*dir) closedir(*dir);
    if ((*dir = opendir(path)) == NULL) {
        log_message("Failed to open %s\n", path);
        perror("opendir");
        return 0;
    }
    return 1;
}

typedef struct {
    const char *attribute_name;
    const char *print_prefix;
    const char *print_postfix;
} Atr_Print_arg;

void print_attribute_value(const Atr_Print_arg arg, int y) {
    char path[PATH_MAX];
    char buffer[MAX_READ];

    snprintf(path, sizeof(path), "%s%s/%s", SYSFS_USB_DEVICES, cursor.device_name, arg.attribute_name);
    read_file_content(path, buffer, sizeof(buffer));
    if(!buffer[0]) {
        return;
    }
    mvwprintw(right_win, y, 1, "%s: %s %s\n",
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

    if(!strlen(ent->d_name)) return 0;
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
    read_file_content(path, buffer, sizeof(buffer));
    unsigned long int size = 0;
    sscanf(buffer, "%ld", &size);

    mvwprintw(right_win, il_info.curr_y++, 1, "Size: %ld MB\n", size * 512 / 1024 / 1024);

    snprintf(path, sizeof(path), "%s%s/%s", SYSFS_USB_DEVICES, cursor.device_name, "bMaxPower");
    if (access(path, F_OK) == 0) {
        read_file_content(path, buffer, sizeof(buffer));
        mvwprintw(right_win, il_info.curr_y++, 1, "Max Power: %s\n", buffer);
    }
    if (dir) closedir(dir);
}

int is_storage_device(const char *devpath) {
    char path[PATH_MAX];
    char buffer[MAX_READ];
    char iface_path[PATH_MAX];

    snprintf(path, sizeof(path), "%s%s/bDeviceClass", SYSFS_USB_DEVICES, devpath);
    read_file_content(path, buffer, sizeof(buffer));
    if (strstr("08", buffer)) {
        return 1;
    }
    

    DIR *dir;
    struct dirent *ent;
    snprintf(path, sizeof(path), "%s%s", SYSFS_USB_DEVICES, devpath);
    
    if ((dir = opendir(path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, devpath, strlen(devpath)) == 0 && 
                strchr(ent->d_name, ':') != NULL) {
                
                snprintf(iface_path, sizeof(iface_path), "%s/%s/bInterfaceClass", path, ent->d_name);
                read_file_content(iface_path, buffer, sizeof(buffer));
                if (strstr("08", buffer)) {
                    closedir(dir);
                    return 1;
                }
            }
        }
        if (dir) closedir(dir);
    }
    return 0;    
}


void get_mount_points(const char* device_name) {
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

    char path[PATH_MAX];
    char buffer[MAX_READ];
    
    Atr_Print_arg args[] = {
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
        print_attribute_value(args[count], il_info.curr_y++);
    }


    if (is_storage_device(cursor.device_name)) {
        print_storage_device_info();
        mvwprintw(right_win, il_info.curr_y++, 1, "Block name: %s", cursor.block_name);

        get_mount_points(cursor.block_name);
    }
    box(right_win, 0, 0);
    wrefresh(right_win);
}


int is_usb_device(const char *name) {
    if (strchr(name, ':') != NULL) return 0;
    if (strncmp(name, "usb", 3) == 0) return 0;
    if (strchr(name, '-') != NULL) return 1;
    return 0;
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
                wattron(left_win, COLOR_PAIR(SELECTED_COLOR) | A_REVERSE);
                mvwprintw(left_win, y++, 1, "%s", ent->d_name);
                wattroff(left_win, COLOR_PAIR(SELECTED_COLOR) | A_REVERSE);
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
        msleep(2000);
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




int main() {
    log_init();
    log_message("Program started");

    initscr();        
    cbreak();           
    noecho();           
    keypad(stdscr, TRUE); 
    nodelay(stdscr, TRUE); 
    curs_set(0);

    left_win = newwin(LINES - 2, COLS / 2, 1, 0);
    right_win = newwin(LINES - 2, COLS / 2, 1, COLS / 2);
    bottom_win = newwin(1, COLS, LINES - 1, 1);

    cursor.window = device_list;
    cursor.y = WIDOW_TOP_PADDING;
    cursor.mount_path[0] = 0;
    while (1) {
        int ch = getch();
        if (ch == 'q') break;  
        if (ch == 274) break;  //F10

        if (ch == KEY_RESIZE) {

            endwin();
            refresh();
            clear();

            resize_term(0, 0);
            werase(left_win);
            werase(right_win);
            werase(bottom_win);
            left_win = newwin(LINES - 2, COLS / 2, 1, 0);
            right_win = newwin(LINES - 2, COLS / 2, 1, COLS / 2);
            bottom_win = newwin(1, COLS, LINES - 1, 1);
        }
        if (ch == KEY_DOWN && cursor.y <= dl_info.len + WIDOW_TOP_PADDING) {
            if (cursor.y == dl_info.len + WIDOW_TOP_PADDING - 1) 
                cursor.y = WIDOW_TOP_PADDING;
            else 
                cursor.y++;
            cursor.mount_path[0] = 0;
        }
        if (ch == KEY_UP && cursor.y >= WIDOW_TOP_PADDING) {
            
            if(cursor.y == WIDOW_TOP_PADDING)
                cursor.y = dl_info.len + WIDOW_TOP_PADDING - 1;
            else
                cursor.y--;
            cursor.mount_path[0] = 0;
        }
        if(cursor.y > dl_info.len + WIDOW_TOP_PADDING - 1 && cursor.y > WIDOW_TOP_PADDING)
            cursor.y--;

        if(ch == 266 && is_storage_device(cursor.device_name)) {//F2
            test_storage(cursor.mount_path, 100);
        }
        refresh();
        clear();


        draw_left_window();
        draw_right_window();
        draw_bottom_window();

        msleep(50);
    }
    delwin(left_win);
    delwin(right_win);
    delwin(bottom_win);
    endwin();  
    return 0;
}