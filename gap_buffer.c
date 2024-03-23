#include "gap_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

const size_t INITIAL_GAP_SIZE = 1024;

/* POST:
 * data = [         source]
 *         ^  gap  ^
 */
GapBuffer *gb_new(wchar_t source[static 1]) {
    GapBuffer *gb = malloc(sizeof(GapBuffer));
    size_t srclen = wcslen(source) + 1;
    wchar_t *data = malloc((INITIAL_GAP_SIZE + srclen) * sizeof(wchar_t));
    memcpy(data + INITIAL_GAP_SIZE, source, srclen * wsize);
    *gb = (GapBuffer){.data = data,
                      .gap_start = 0,
                      .gap_end = INITIAL_GAP_SIZE * wsize,
                      .gap_size = INITIAL_GAP_SIZE * wsize,
                      .md = md_new(source)};
    return gb;
}

wchar_t *gb_get_chars(GapBuffer *gb, size_t start_row, size_t start_col,
                      size_t end_row, size_t end_col) {
    // TODO: finish this
    // size_t offset_start = md_get_offset(&gb->md, start_row) + start_col;
    // if (offset_start > gb->gap_start) {
    //     offset_start += gb->gap_size;
    // }
    // size_t offset_end = md_get_offset(&gb->md, end_row) + end_col;
    // if (offset_end > gb->gap_end) {
    //     offset_end += gb->gap_size;
    // }
}

void gb_insert(GapBuffer *gb, size_t row, size_t col, wchar_t *wc);

void gb_search(GapBuffer *gb, char *needle);

void gb_free(GapBuffer *gb) {
    free(gb->data);
    free(gb->md);
    free(gb);
}
