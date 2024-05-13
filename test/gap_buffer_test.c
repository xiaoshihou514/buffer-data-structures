#include "../gap_buffer.h"
#include "criterion/logging.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <wchar.h>

GapBuffer *gb;
wchar_t *src =
    L"A gap buffer in computer science \nis a dynamic array that allows "
    L"efficient insertion and \ndeletion operations clustered near the same "
    L"location.\n Gap buffers \nare especially common in text editors, where "
    L"most changes to \nthe text occur at or near the current \nlocation of "
    L"the cursor. The text is\n\n stored in a large \nbuffer in two contiguous "
    L"segments, with a gap \nbetween them for inserting new text. Moving \nthe "
    L"cursor involves copying text\n from one side of the \ngap to the other "
    L"(sometimes copying is delayed until the next operation \nthat \nchanges "
    L"the text). Insertion adds new text \nat the\n end of \nthe first "
    L"segment; deletion deletes it.\n";

void new_gap_buffer(void) { gb = gb_new(src); }

void free_gap_buffer(void) { gb_free(gb); }

TestSuite(gap_buffer, .init = new_gap_buffer, .fini = free_gap_buffer);

Test(gap_buffer, new) {
    const size_t INITIAL_GAP_SIZE = 1024;

    cr_expect_eq(gb->gap_start, 0);
    cr_expect_eq(gb->gap_end, INITIAL_GAP_SIZE);
    cr_expect_eq(gb->gap_size, INITIAL_GAP_SIZE);

    for (size_t i = 0; i < wcslen(src); i++) {
        cr_expect_eq(gb->data[i + INITIAL_GAP_SIZE], src[i]);
    }
}

Test(gap_buffer, get) {
    wchar_t *substring = gb_get_chars(gb, 1, 1, 2, 2);
    const wchar_t *expected = L"A gap buffer in computer science \nis";
    cr_log_warn("%ls", substring);
    cr_expect_eq(wcslen(expected), wcslen(substring));
    cr_expect_eq(0, wcscmp(expected, substring));
    free(substring);
}
