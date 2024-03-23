#include "metadata.h"
#include <wchar.h>

typedef struct {
    wchar_t *data;
    size_t gap_start;
    size_t gap_end;
    size_t gap_size;
    MetaData *md;
} GapBuffer;

GapBuffer *gb_new(wchar_t source[static 1]);

wchar_t *gb_get_chars(GapBuffer *gb, size_t start_row, size_t start_col,
                      size_t end_row, size_t end_col);

void gb_insert(GapBuffer *gb, size_t row, size_t col, wchar_t *wc);

void gb_search(GapBuffer *gb, char *needle);

void gb_free(GapBuffer *gb);

#define wsize sizeof(wchar_t)
