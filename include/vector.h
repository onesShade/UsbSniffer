#ifndef VECTOR
#define VECTOR
#include <stdlib.h>

typedef struct {
    void* data;
    size_t size;
    size_t capacity;
    size_t element_size;
} Vector;

Vector* vector_init(size_t elem_size, size_t initial_size);

void vector_free(Vector* v);

void vector_push_back(Vector* v, const void* item);

void* vector_at(Vector* v, size_t index);

void vector_clear(Vector* v);

void vector_sort(Vector* v, __compar_fn_t compare);

int str_compare_fun(const void *a, const void *b);

int str_compare_second_substr_fun(const void *a, const void *b);

#endif