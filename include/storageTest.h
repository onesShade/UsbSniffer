#ifndef STORAGE_TEST_H
#define STORAGE_TEST_H

#include "globals.h"

extern char testPropsStr[MAX_READ];

void set_test_props();
void test_storage(const char *mount_point);
void update_st_test_settings(int key);


#endif