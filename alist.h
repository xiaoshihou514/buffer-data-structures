#include <stddef.h>

// specialized array list for int
typedef struct {
    int *data;
    int size;
    int used;
} ArrayList;

ArrayList *alist_new();

void alist_push(ArrayList alist[static 1], int item);

void alist_free(ArrayList alist[static 1]);
