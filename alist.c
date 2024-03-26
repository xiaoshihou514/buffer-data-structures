#include "alist.h"
#include <stdlib.h>

const int ALIST_INITIAL_SIZE = 32;

ArrayList *alist_new() {
    ArrayList *result = malloc(sizeof(ArrayList));
    result->data = malloc(sizeof(size_t) * ALIST_INITIAL_SIZE);
    result->size = ALIST_INITIAL_SIZE;
    result->used = 0;
    return result;
}

void alist_push(ArrayList alist[static 1], ssize_t item) {
    if (alist->used == alist->size) {
        // needs to resize
        alist->size *= 2;
        alist->data = realloc(alist->data, alist->size * sizeof(size_t));
    }
    alist->data[alist->used] = item;
    alist->used++;
}

void alist_free(ArrayList *alist) {
    if (alist) {
        free(alist->data);
        free(alist);
    }
}
