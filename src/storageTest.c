#define _POSIX_C_SOURCE 199309L 

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

#define TEST_FILE_NAME "storage_test.bin"
void test_storage(const char *mount_point, int size_mb) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", mount_point, TEST_FILE_NAME);
    struct timespec start, end;
    double elapsed_sec;

    long seed = time(NULL);
    printf("Creating temp test file of %d mb at %s...\r\n", size_mb, mount_point);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        fprintf(stderr, "File open exception: %s\r\n", strerror(errno));
        return;
    }

    size_t total_size = (size_t)size_mb * 1024 * 1024;
    char *buffer = malloc(total_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation exception\r\n");
        close(fd);
        return;
    }

    srand(seed);
    for (size_t i = 0; i < total_size; i++) {
        buffer[i] = rand() % 256;
    }

    clock_gettime(CLOCK_REALTIME, &start);

    ssize_t written = write(fd, buffer, total_size);
    if (written != total_size) {
        fprintf(stderr, "Write exception: %s\r\n", strerror(errno));
        free(buffer);
        close(fd);
        return;
    }

    free(buffer);
    fsync(fd); 
    close(fd);

       
    clock_gettime(CLOCK_REALTIME, &end);
    elapsed_sec = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) * 1.0e-9;
    printf("Write time: %.3f sec\r\n", elapsed_sec);
    printf("Write speed: %.3f mb/s\r\n", (double)size_mb / elapsed_sec);

    printf("Reading and examination...\r\n");

    fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "File open exception: %s\r\n", strerror(errno));
        return;
    }

    buffer = malloc(total_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation exception\r\n");
        close(fd);
        return;
    }

    clock_gettime(CLOCK_REALTIME, &start);
    int errors = 0;
    ssize_t bytes_read = read(fd, buffer, total_size);
    if (bytes_read != total_size) {
        fprintf(stderr, "Read exception: %s\r\n", strerror(errno));
    }
    srand(seed);
    for (size_t j = 0; j < total_size; j++) {
        char expected = rand() % 256;
        if (buffer[j] != expected) {
            errors++;
        }
    }


    free(buffer);
    close(fd);

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed_sec = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) * 1.0e-9;
    printf("Read time: %.3f sec\r\n", elapsed_sec);
    printf("Read speed: %.3f mb/s\r\n", (double)size_mb / elapsed_sec);


    if (errors > 0) {
        printf("Errors found: %d\r\n", errors);
    } else {
        printf("Test passed!\r\n");
    }

    unlink(file_path);

    printf("Press space to exit...\r\n");
    while (getch() != ' ');

}