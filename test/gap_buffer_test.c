#include "../gap_buffer.h"
#include <criterion/criterion.h>
#include <wchar.h>

GapBuffer *gb;
wchar_t *src = L"Hello, gap buffer!";

void new_gap_buffer(void) { gb = gb_new(src); }

void free_gap_buffer(void) { gb_free(gb); }

TestSuite(gap_buffer, .init = new_gap_buffer, .fini = free_gap_buffer);

Test(gap_buffer, new) {
    const size_t INITIAL_GAP_SIZE = 1024;

    cr_expect_eq(gb->gap_start, 0);
    cr_expect_eq(gb->gap_end, INITIAL_GAP_SIZE * wsize);
    cr_expect_eq(gb->gap_size, INITIAL_GAP_SIZE * wsize);

    for (size_t i = 0; i < wcslen(src); i++) {
        cr_expect_eq(gb->data[i + INITIAL_GAP_SIZE], src[i]);
    }
}
