#include <stddef.h>

// specialized array list for size_t
typedef struct {
    size_t *data;
    size_t size;
    size_t used;
} ArrayList;

ArrayList *alist_new();

void alist_push(ArrayList alist[static 1],size_t item);

void alist_free(ArrayList alist[static 1]);
