#include <string.h>

#include "util.h"
#include "vector.h"


Vector* vector_init(size_t elem_size, size_t initial_size) {
    Vector* v = malloc(sizeof(Vector));
    v->size = 0;
    v->capacity = initial_size;
    v->data = NULL;
    if(initial_size) {
        v->data = malloc(initial_size * elem_size);
        if(!v->data) {
            vector_free(v);
            log_message("Malloc vector create exepion");
            return 0;
        }
    }
    v->element_size = elem_size;
    return v;
}

void vector_free(Vector* v) {
    free(v->data);
    free(v);
}

void vector_push_back(Vector* v, const void* item) {
    if (v->size >= v->capacity) {
        size_t new_capacity = v->capacity == 0 ? 1 : v->capacity * 2;
        void* new_data = realloc(v->data, new_capacity * v->element_size);
        v->data = new_data;
        v->capacity = new_capacity;
    }
    memcpy((char*)v->data + v->size * v->element_size, item, v->element_size);
    v->size++;
}

void* vector_at(Vector* v, size_t index) {
    if (index >= v->size) return NULL;
    return v->data + index * v->element_size;
}

void vector_clear(Vector* v) {
    v->size = 0;
}