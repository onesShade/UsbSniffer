#include <linux/limits.h>
#define _POSIX_C_SOURCE 199309L 
#include "vector.h"

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
#include <ctype.h>

#include "defines.h"
#include "globals.h"

TestProps testProps;
char testPropsStr[MAX_READ];

void set_test_props() {
    sscanf(vector_at(test_size_sel_dl->entryes, test_size_sel_dl->selected), 
            "%d", &testProps.data_size);
    sscanf(vector_at(test_passes_dl->entryes, test_passes_dl->selected), 
            "%d", &testProps.number_of_passes);


    snprintf(testPropsStr, MAX_READ, "Press [ENTER] to test on %d MB file %d times", testProps.data_size, testProps.number_of_passes);
}

void print_loading_bar(int done, int all) {
    float percentage = (float)done / (float)all;
    char bar[32];

    bar[4] = '[';
    for(int i = 0; i < 10; i++)
        if(percentage > 1.f / i)
            bar[5+i] = '=';
        else
            bar[5+i] = ' ';
    bar[14] = ']';
    printf("%s\r", bar);
}


void test_write(const char* path, char* data) {
    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "File open exception: %s\n", strerror(errno));
        return;
    }

    size_t total_size = (size_t)testProps.data_size * 1024 * 1024;

    size_t written = fwrite(data, 1, total_size, file);
    if (written != total_size) {
        fprintf(stderr, "Write exception: %s\n", strerror(errno));
        fclose(file);
        return;
    }

    fflush(file);
    fclose(file);
}

void test_read(const char* path, char* out) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "File open exception: %s\n", strerror(errno));
        return;
    }

    size_t total_size = (size_t)testProps.data_size * 1024 * 1024;

    size_t bytes_read = fread(out, 1, total_size, file);
    if (bytes_read != total_size) {
        if (feof(file)) {
            fprintf(stderr, "Unexpected end of file\n");
        } else if (ferror(file)) {
            fprintf(stderr, "Read exception: %s\n", strerror(errno));
        }
    }

    fclose(file);
}

void run_w_r_test(const char *mount_point) {
    char file_path[PATH_MAX];

    snprintf(file_path, sizeof(file_path), "%s/%s", mount_point, TEST_FILE_NAME);
    struct timespec start, end;
    double elapsed_sec;

    long seed = time(NULL);
    printf("Creating temp test file of %d MB %d times at %s\r\n", testProps.data_size, testProps.number_of_passes, file_path);

    size_t total_size = (size_t)testProps.data_size * 1024 * 1024;
    char* buffer = malloc(total_size);
    char* out_data = malloc(total_size);

    srand(seed);
    for (size_t i = 0; i < total_size; i++) {
        buffer[i] = rand() % 256;
    }

    clock_gettime(CLOCK_REALTIME, &start);
    for(int i = 0; i < testProps.number_of_passes; i++) {
        printf("[%d/%d] w\r\n", i + 1, testProps.number_of_passes);
        test_write(file_path, buffer);
    }
    clock_gettime(CLOCK_REALTIME, &end);

    elapsed_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1.0e-9;
    printf("Write time: %.3f sec\r\n", elapsed_sec);
    printf("Write speed: %.3f MB/s\r\n", (double)testProps.data_size * testProps.number_of_passes / elapsed_sec);

    printf("Reading and verification...\r\n");
    clock_gettime(CLOCK_REALTIME, &start);
    test_read(file_path, out_data);
    clock_gettime(CLOCK_REALTIME, &end);

    elapsed_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1.0e-9;
    printf("Read time: %.3f sec\r\n", elapsed_sec);
    printf("Read speed: %.3f MB/s\r\n", (double)testProps.data_size / elapsed_sec);


    int errors = 0;
    for (size_t j = 0; j < total_size; j++) {
        if (buffer[j] != out_data[j]) {
            errors++;
        }
    }

    free(buffer);
    free(out_data);

    if (errors > 0) {
        printf("Errors found: %d\r\n", errors);
    } else {
        printf("Test passed!\r\n");
    }

    printf("Press [ENTER] to exit...\r\n");
    while (getchar() != '\r');

    if (remove(file_path)) {
        fprintf(stderr, "Failed to delete test file: %s\r\n", strerror(errno));
    }
}


void use_octal_escapes(char* str) {
    char buffer[PATH_MAX];

    int sp = 0;
    int dp = 0;
    int og_len = strlen(str);

    for(; sp < og_len; sp++, dp++) {
        if (str[sp] == '\\' 
            && isdigit(str[sp + 1])
            && isdigit(str[sp + 2])
            && isdigit(str[sp + 3])) {
                
            char simbol = (str[sp + 1] - '0') * 64 
                        + (str[sp + 2] - '0') * 8 
                        + (str[sp + 3] - '0');
            buffer[dp] = simbol;
            sp += 3; 
        } else {
            buffer[dp] = str[sp];
        }
    }
    buffer[dp] = 0;
    strncpy(str, buffer, PATH_MAX);
}

void update_st_test_settings(int key) {
    if (key == 'q') {
        selection_lw.window = device_list;
    }
    if (key == 'm') {
        dl_iterate(mount_point_dl, +1);
    }
    if (key == 'b') {
        dl_iterate(test_size_sel_dl, +1);
    }
    if (key == 'n') {
        dl_iterate(test_passes_dl, +1);
    }
    if (key == '\n') {
        run_w_r_test(vector_at(mount_point_dl->entryes, mount_point_dl->selected));
        selection_lw.window = device_list;
    }
    set_test_props();
}