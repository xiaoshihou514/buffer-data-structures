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
                      .total_size = srclen + INITIAL_GAP_SIZE,
                      .md = md_new(source)};
    return gb;
}

/* 1-indexed, inclusive */
wchar_t *gb_get_chars(GapBuffer *gb, size_t start_row, size_t start_col,
                      size_t end_row, size_t end_col) {
    // get grapheme offset
    size_t offset_start = md_get_line_start(gb->md, start_row) + start_col;
    size_t offset_end = md_get_line_start(gb->md, end_row) + end_col + 1;
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

void gb_insert(GapBuffer *gb, size_t row, size_t col, wchar_t *wc) {
    size_t size = wcslen(wc);
    if (size > gb->gap_size) {
        // not enough space, we double gap size until we have enough
        size_t oldsize = gb->gap_size;
        while (size >= gb->gap_size) {
            gb->gap_size *= 2;
        }

        // actually allocate it
        size_t alloc_size = gb->gap_size - oldsize;
        size_t tail_size = gb->total_size - gb->gap_end;
        realloc(gb->data + gb->gap_end, (tail_size + alloc_size) * wsize);

        // fix broken state
        memcpy(gb->data + gb->gap_end + gb->gap_size, gb->data + gb->gap_end,
               tail_size * wsize);
        gb->gap_end = gb->gap_start + gb->gap_size;
        gb->total_size += alloc_size;
    }

    // move gap to the correct position
    size_t offset = md_get_line_start(gb->md, row) + col;
    if (offset < gb->gap_start) {
        // offset in the head
        size_t diff = gb->gap_start - offset;
        memcpy(gb->data + gb->gap_end - diff, gb->data + offset, diff * wsize);
    } else if (offset > gb->gap_end) {
        // offset in the tail
        size_t diff = offset - gb->gap_end;
        memcpy(gb->data + gb->gap_end, gb->data + gb->gap_start, diff * wsize);
    } else {
        // offset in the middle of the gap
        size_t diff = offset - gb->gap_start;
        if (gb->total_size - gb->gap_end < diff) {
            // offset greater than end of buffer
            cr_log_error("gb_insert: insert position out of bounds");
            return;
        }
        memcpy(gb->data + gb->gap_start, gb->data + gb->gap_end, diff * wsize);
    }
    gb->gap_start = offset;

    // copy into the gap
    memcpy(gb->data + gb->gap_start, wc, size * wsize);
    gb->gap_start += size;
    gb->gap_size -= size;

    // fix broken metadata
    size_t acc = 0;
    for (size_t i = 0; i < size; i++) {
        switch (wc[i]) {
        case L'\n':
            // shift line by acc
            md_shift_offset(gb->md, row, acc, nullptr);
            md_insert(gb->md, row);
            row++;
            acc = 0;
            break;
        default:
            acc++;
        }
    }
    md_shift_offset(gb->md, row, acc, nullptr);
}

ArrayList *gb_search(GapBuffer *gb, wchar_t *needle);

wchar_t *gb_write(GapBuffer *gb) {
    wchar_t *result = malloc((gb->total_size - gb->gap_size) * wsize);
    memcpy(result, gb->data, gb->gap_start * wsize);
    memcpy(result + gb->gap_start, gb->data + gb->gap_end,
           (gb->total_size - gb->gap_end) * wsize);
    return result;
}

void gb_free(GapBuffer *gb) {
    free(gb->data);
    md_free(gb->md);
    free(gb);
}
