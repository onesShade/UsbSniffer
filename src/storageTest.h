#ifndef STORAGE_TEST_H
#define STORAGE_TEST_H

#include "globals.h"

typedef struct {
    int data_size;
    int number_of_passes; 
} TestProps;

extern TestProps testProps;
extern char testPropsStr[MAX_READ];

void set_test_props();
void test_storage(const char *mount_point);
void use_octal_escapes(char* str);
void update_st_test_settings(int key);


#endif