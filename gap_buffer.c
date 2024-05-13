#include "criterion/logging.h"
#include "gap_buffer.h"
#include "metadata.h"
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
                      .gap_end = INITIAL_GAP_SIZE,
                      .gap_size = INITIAL_GAP_SIZE,
                      .md = md_new(source)};
    return gb;
}

/* 1-indexed, inclusive */
wchar_t *gb_get_chars(GapBuffer *gb, size_t start_row, size_t start_col,
                      size_t end_row, size_t end_col) {
    start_col--;
    // get grapheme offset
    size_t offset_start = md_get_line_start(gb->md, start_row) + start_col;
    size_t offset_end = md_get_line_start(gb->md, end_row) + 1 + end_col;
    cr_log_warn("offset_start: %zu", offset_start);
    cr_log_warn("offset_end: %zu", offset_end);
    if (offset_start > offset_end) {
        cr_log_error("gb_get_chars: invalid range");
        return nullptr;
    }

    size_t size = offset_end - offset_start;
    wchar_t *result = malloc((size + 1) * wsize);

    // memcpy accordingly
    if (offset_end < gb->gap_start) {
        // all before gap
        memcpy(result, gb->data + offset_start, size * wsize);
    } else if (offset_start >= gb->gap_end) {
        // all after gap
        memcpy(result, gb->data + offset_start + gb->gap_size, size * wsize);
    } else if (offset_start < gb->gap_start) {
        offset_end += gb->gap_size;
        size_t first_half = gb->gap_start - offset_start;
        memcpy(result, gb->data + offset_start, first_half * wsize);
        memcpy(result + first_half, gb->data + gb->gap_end,
               (size - first_half) * wsize);
    } else {
        offset_end += gb->gap_size;
        offset_start += gb->gap_size;
        memcpy(result, gb->data + offset_start, size * wsize);
    }

    result[size] = L'\0';
    return result;
}

void gb_insert(GapBuffer *gb, size_t row, size_t col, wchar_t *wc);

ArrayList *gb_search(GapBuffer *gb, char *needle);

void gb_free(GapBuffer *gb) {
    free(gb->data);
    free(gb->md);
    free(gb);
}
