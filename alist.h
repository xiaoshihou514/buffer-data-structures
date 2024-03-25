#include "sys/types.h"
#include <stddef.h>

#ifndef ALIST_H
#define ALIST_H

// specialized array list for int
typedef struct {
    ssize_t *data;
    size_t size;
    size_t used;
} ArrayList;

ArrayList *alist_new();

void alist_push(ArrayList alist[static 1], ssize_t item);

void alist_free(ArrayList *alist);

#endif
