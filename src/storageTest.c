#define _POSIX_C_SOURCE 199309L 
#define _GNU_SOURCE

#include "util.h"
#include <linux/limits.h>

#include "dispayList.h"
#include "storageTest.h"

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
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "renderer.h"
#include "engine.h"

#include "defines.h"
#include "globals.h"

#define BAR_MAX_LEN 256

typedef struct {
    int data_size;
    int number_of_passes;
    int block_size;
} TestProps;

TestProps testProps;

char testPropsStr[MAX_READ];

typedef enum {
    WRITE_SPEED,
    READ_SPEED,
    RANDOM_READ_SPEED,
    TEST_NONE,
} TestMode;

const char* TestModeStr[] = {
    [WRITE_SPEED] = "WRITE SPEED TEST",
    [READ_SPEED]  = "READ SPEED TEST",
    [RANDOM_READ_SPEED] = "RANDOM READ SPEED TEST",
    [TEST_NONE] = "ERROR",
};

void set_test_props() {
    if (!dl_get_selected(mount_point_dl)) {
        snprintf(testPropsStr, MAX_READ, "No mount point selected.");
        return;
    }

    sscanf(dl_get_selected(test_size_sel_dl), "%d", &testProps.data_size);
    sscanf(dl_get_selected(test_passes_sel_dl), "%d", &testProps.number_of_passes);
    sscanf(dl_get_selected(test_block_size_sel_dl), "%d", &testProps.block_size);
    testProps.block_size *= 1024;

    snprintf(testPropsStr, MAX_READ, "Press [ENTER] to test on %d MB file %d times", testProps.data_size, testProps.number_of_passes);
}

void print_loading_bar(int done, int all, char* str) {
    float percentage = (float)done / (float)all;
    char bar[BAR_MAX_LEN] = {0};

    int rows, cols;
    getmaxyx(popup_win, rows, cols);  

    snprintf(bar, BAR_MAX_LEN, "%05d/%05d [", done, all);
    const int PREFIX_SIZE = 13;
    int BAR_SIZE = (int)((float)cols * 0.7f);

    for (int i = 0; i < BAR_SIZE; i++) {
        if ((float)i/((float)BAR_SIZE) < percentage)
            bar[i + PREFIX_SIZE] = '=';
        else
            bar[i + PREFIX_SIZE] = ' ';
    }
    bar[BAR_SIZE + PREFIX_SIZE] = ']';
    bar[BAR_SIZE + PREFIX_SIZE + 1] = '\0';
    
    s_strcpy(str, bar, BAR_MAX_LEN - 1);
    str[127] = '\0';
}

int test_write(const char* path, const char* data, size_t total_size) {
    void* aligned_buf;
    if (posix_memalign(&aligned_buf, 4096, total_size)) {
        log_message("Memory alignment failed\n");
        return 0;
    }
    memcpy(aligned_buf, data, total_size);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC);
    if (fd == -1) {
        log_message("Open error: %s", strerror(errno));
        free(aligned_buf);
        return 0;
    }

    ssize_t written = write(fd, aligned_buf, total_size);
    if (written == -1) {
        log_message("Write error: %s", strerror(errno));
    }

    free(aligned_buf);
    close(fd);
    return 1;
}

int test_read(const char* path, char* out, size_t total_size) {
    void* aligned_buf;
    if (posix_memalign(&aligned_buf, 4096, total_size)) {
        fprintf(stderr, "Memory alignment failed\n");
        return 0;
    }

    int fd = open(path, O_RDONLY | O_DIRECT);
    if (fd == -1) {
        fprintf(stderr, "Open error: %s\n", strerror(errno));
        free(aligned_buf);
        return 0;
    }

    ssize_t bytes_read = read(fd, aligned_buf, total_size);
    if (bytes_read == -1) {
        fprintf(stderr, "Read error: %s\n", strerror(errno));
    } else {
        memcpy(out, aligned_buf, bytes_read);
    }

    free(aligned_buf);
    close(fd);
    return 1;
}

void run_ws_test(char* file_path, char* buffer, size_t total_size) {
    double elapsed_sec;
    struct timespec start, end;
    dl_add_entry(test_screen_dl, DLEP_NONE, "");
    DLE* bar_dle = dl_add_entry(test_screen_dl, DLEP_CENTERED, "[]");
    dl_add_entry(test_screen_dl, DLEP_NONE, "");
    print_loading_bar(0, testProps.number_of_passes, bar_dle->body);
    draw_all_windows();

    clock_gettime(CLOCK_REALTIME, &start);
    for(int i = 0; i < testProps.number_of_passes; i++) {
        if (!test_write(file_path, buffer, total_size)) {
            while (getchar() != '\r');
            return;
        }
        print_loading_bar(i + 1, testProps.number_of_passes, bar_dle->body);
        draw_all_windows();
    }
    print_loading_bar(testProps.number_of_passes, testProps.number_of_passes, bar_dle->body);
    clock_gettime(CLOCK_REALTIME, &end);

    elapsed_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1.0e-9;
    dl_add_entry(test_screen_dl, DLEP_NONE, "Write time: %.3f sec", 
        elapsed_sec);
    dl_add_entry(test_screen_dl, DLEP_NONE, "Write speed: %.3f MB/s", 
        (double)testProps.data_size * testProps.number_of_passes / elapsed_sec);
    draw_all_windows();        
}

void run_rs_test(char* file_path, char* buffer, size_t total_size, char* out_data) {
    struct timespec start, end;
    double elapsed_sec;

    if (!test_write(file_path, buffer, total_size)) {
        return;
    }

    dl_add_entry(test_screen_dl, DLEP_NONE, "");
    DLE* bar_dle = dl_add_entry(test_screen_dl, DLEP_CENTERED, "[]");
    dl_add_entry(test_screen_dl, DLEP_NONE, "");
    print_loading_bar(0, testProps.number_of_passes, bar_dle->body);
    draw_all_windows();

    clock_gettime(CLOCK_REALTIME, &start);
    for(int i = 0; i < testProps.number_of_passes; i++) {
        if(!test_read(file_path, out_data, total_size)) {
            while (getchar() != '\r');
            return;
        }
        print_loading_bar(i + 1, testProps.number_of_passes, bar_dle->body);
        draw_all_windows();
    }
    clock_gettime(CLOCK_REALTIME, &end);

    elapsed_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1.0e-9;
    dl_add_entry(test_screen_dl, DLEP_NONE, "Read time: %.3f sec", 
        elapsed_sec);
    dl_add_entry(test_screen_dl, DLEP_NONE, "Read speed: %.3f MB/s", 
        (double)testProps.data_size * testProps.number_of_passes / elapsed_sec);
    draw_all_windows();   
}

void run_random_read_test(char* file_path, char* buffer, size_t total_size) {
    void* aligned_buf;
    if (posix_memalign(&aligned_buf, 4096, testProps.block_size)) {
        log_message("Memory alignment failed", strerror(errno));
        return;
    }

    if (!test_write(file_path, buffer, total_size)) 
        return;

    int fd = open(file_path, O_RDONLY | O_DIRECT);
    if (fd == -1) {
        log_message("Open error", strerror(errno));
        free(aligned_buf);
        return;
    }

    struct timespec start, end;
    double elapsed_sec;
    
    dl_add_entry(test_screen_dl, DLEP_NONE, "");
    DLE* bar_dle = dl_add_entry(test_screen_dl, DLEP_CENTERED, "[]");
    dl_add_entry(test_screen_dl, DLEP_NONE, "");
    print_loading_bar(0, testProps.number_of_passes, bar_dle->body);
    draw_all_windows();

    clock_gettime(CLOCK_REALTIME, &start);
    for (int i = 0; i < testProps.number_of_passes; i++) {
        off_t offset = (rand() % (total_size / testProps.block_size)) * testProps.block_size;
        if (lseek(fd, offset, SEEK_SET) == -1) {
            log_message("Seek error: %s", strerror(errno));
            break;
        }
        
        ssize_t bytes_read = read(fd, aligned_buf, testProps.block_size);
        if (bytes_read == -1) {
            log_message("Read error: %s", strerror(errno));
            break;
        }
        
        print_loading_bar(i + 1, testProps.number_of_passes, bar_dle->body);
        if (i % 10 == 0) {
            draw_all_windows();
        }
    }
    clock_gettime(CLOCK_REALTIME, &end);

    elapsed_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1.0e-9;
    dl_add_entry(test_screen_dl, DLEP_NONE, "Random read time: %.3f sec", elapsed_sec);
    dl_add_entry(test_screen_dl, DLEP_NONE, "Random read speed: %.3f MB/s", 
        (testProps.number_of_passes * testProps.block_size) / (elapsed_sec * 1024 * 1024));
    draw_all_windows();        
    free(aligned_buf);
    close(fd);
}

void run_general_test(TestMode tm) {
    const char *mount_point = dl_get_selected(mount_point_dl);
    if(!mount_point) {
        return;
    }

    char file_path[PATH_MAX];

    snprintf(file_path, sizeof(file_path), "%s/%s", mount_point, TEST_FILE_NAME);
    struct timespec start, end;

    dl_add_entry(test_screen_dl, DLEP_CENTERED, "RUNNING %s", TestModeStr[tm]);

    long seed = time(NULL);

    dl_add_entry(test_screen_dl, DLEP_NONE, 
        "Generating temp test data of %d MB", 
        testProps.data_size);
    draw_all_windows();        
    size_t total_size = (size_t)testProps.data_size * 1024 * 1024;
    char* buffer = malloc(total_size);
    char* out_data = malloc(total_size);

    dl_add_entry(test_screen_dl, DLEP_NONE, 
        "Creating temp test file of %d MB %d times at", 
        testProps.data_size, testProps.number_of_passes);
    dl_add_entry(test_screen_dl, DLEP_NONE, "%s", file_path);
    draw_all_windows();        

    srand(seed);
    for (size_t i = 0; i < total_size; i++) {
        buffer[i] = rand() % 256;
    }

    switch (tm) {
        case WRITE_SPEED:
            run_ws_test(file_path, buffer, total_size);
            break;
        case READ_SPEED:
            run_rs_test(file_path, buffer, total_size, out_data);
            break;
        case RANDOM_READ_SPEED:
            run_random_read_test(file_path, buffer, total_size);
            break;
        default:
            break;
    }
    
    dl_add_entry(test_screen_dl, DLEP_NONE, "File verification...");
    draw_all_windows();        

    if(!test_read(file_path, out_data, total_size))
        return;

    int errors = 0;
    for (size_t j = 0; j < total_size; j++) {
        if (buffer[j] != out_data[j]) {
            errors++;
        }
    }

    free(buffer);   
    free(out_data);

    if (errors > 0) {
        dl_add_entry(test_screen_dl, DLEP_NONE, 
            "Errors found: %d\r\n", errors);
    } else {
        dl_add_entry(test_screen_dl, DLEP_NONE, "Test passed!");
    }

    dl_add_entry(test_screen_dl, DLEP_NONE, "Press [Q] to exit...");
    draw_all_windows();        
    
    set_current_window(storage_test_results);
    while (selection_lw.window == storage_test_results) {
        int key = getch();
        update_keys(key);
        draw_all_windows();      
        msleep(MAIN_LOOP_SLEEP_TIME_MS);
    };

    if (unlink(file_path)) {
        log_message("Failed to delete test file: %s", strerror(errno));
    }
}

void update_st_test_settings(int key) {
    dl_reset_sel_pos(mount_point_dl);
    if (key == 'q') {
        set_current_window(device_list);
    }
    if (key == 'm') {
        dl_iterate(mount_point_dl, +1);
    }
    if (key == 'b') {
        dl_iterate(test_size_sel_dl, +1);
    }
    if (key == 'n') {
        dl_iterate(test_passes_sel_dl, +1);
    }
    if (key == 'v') {
        dl_iterate(test_mode_sel_dl, +1);
        update_sel_dls();
    }
    if (key == 'l') {
        dl_iterate(test_block_size_sel_dl, +1);
        update_sel_dls();
    }

    if (key == '\n') {
        set_current_window(storage_test_run);
        dl_clear(test_screen_dl);

        TestMode tm = TEST_NONE;
        if (strncmp("WS", dl_get_selected(test_mode_sel_dl), MAX_READ) == 0) {
            tm = WRITE_SPEED;
        }
        
        if (strncmp("RS", dl_get_selected(test_mode_sel_dl), MAX_READ) == 0) {
            tm = READ_SPEED;
        }

        if (strncmp("RR", dl_get_selected(test_mode_sel_dl), MAX_READ) == 0) {
            tm = RANDOM_READ_SPEED;
        }

        if (tm != TEST_NONE) {
            log_message("Running %s", TestModeStr[tm]);
            run_general_test(tm);
        } else {
            log_message("Test selection failed. test_mode_sel_dl = %s", dl_get_selected(test_mode_sel_dl));
        }
        set_current_window(device_list);
    }
    set_test_props();
}