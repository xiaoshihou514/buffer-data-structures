#include "../gap_buffer.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <stdlib.h>
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

#define assert_eq_sz(...) cr_assert(eq(sz, ##__VA_ARGS__))
#define assert_eq_wcs(...) cr_assert(eq(wcs, ##__VA_ARGS__))

Test(gap_buffer, new) {
    const size_t INITIAL_GAP_SIZE = 1024;

    assert_eq_sz(gb->gap_start, 0);
    assert_eq_sz(gb->gap_end, INITIAL_GAP_SIZE);
    assert_eq_sz(gb->gap_size, INITIAL_GAP_SIZE);

    for (size_t i = 0; i < wcslen(src); i++) {
        cr_expect_eq(gb->data[i + INITIAL_GAP_SIZE], src[i]);
    }
}

#define test_gb_get(srow, scol, erow, ecol, out)                               \
    substring = gb_get_chars(gb, srow, scol, erow, ecol);                      \
    expected = out;                                                            \
    assert_eq_wcs(substring, expected);                                        \
    free(substring);

Test(gap_buffer, get) {
    wchar_t *substring;
    wchar_t *expected;
    test_gb_get(1, 1, 2, 2, L"A gap buffer in computer science \nis");
    test_gb_get(5, 10, 6, 38,
                L"ially common in text editors, where most changes to \nthe "
                L"text occur at or near the current ");
    test_gb_get(5, 10, 6, 39,
                L"ially common in text editors, where most changes to \nthe "
                L"text occur at or near the current \n");
    test_gb_get(5, 10, 7, 0,
                L"ially common in text editors, where most changes to \nthe "
                L"text occur at or near the current \n");
    test_gb_get(17, 0, 18, 1, L"\nat the\n ");
    test_gb_get(10, 8, 10, 25, L"in two contiguous ");
    test_gb_get(3, 10, 11, 5,
                L"operations clustered near the same location.\n Gap buffers "
                L"\nare especially common in text editors, where most changes "
                L"to \nthe text occur at or near the current \nlocation of the "
                L"cursor. The text is\n\n stored in a large \nbuffer in two "
                L"contiguous segments, with a gap \nbetwe");
}

Test(gap_buffer, write) { assert_eq_wcs(gb_write(gb), src); }

// TODO
#define test_gb_insert(row, col, wc)                                           \
    substr = malloc(4096 * wsize);                                             \
    expected = malloc(4096 * wsize);                                           \
    /* record pre state */                                                     \
    inserted = wc;                                                             \
    str_pre = gb_write(gb);                                                    \
    offset = md_get_line_start(gb->md, row) + col;                             \
                                                                               \
    /* record pre state */                                                     \
    inserted = wc;                                                             \
    str_pre = gb_write(gb);                                                    \
    offset = md_get_line_start(gb->md, row) + col;                             \
    gb_insert(gb, row, col, wc);                                               \
                                                                               \
    /* record post state */                                                    \
    str = gb_write(gb);                                                        \
                                                                               \
    /* test str_pre[..offset) == str[..offset) */                              \
    wcsncpy(substr, str, offset);                                              \
    substr[offset] = L'\0';                                                    \
    wcsncpy(expected, str_pre, offset);                                        \
    expected[offset] = L'\0';                                                  \
    assert_eq_wcs(substr, expected);                                           \
                                                                               \
    /* test str[offset..offset+len) == inserted */                             \
    len = wcslen(inserted);                                                    \
    wcsncpy(substr, str + offset, len);                                        \
    substr[len] = L'\0';                                                       \
    assert_eq_wcs(substr, inserted);                                           \
                                                                               \
    /* test str[offset+len..) == str_pre[offset..) */                          \
    wcsncpy(substr, str + offset + len, wcslen(str) - (offset + len));         \
    substr[wcslen(str) - (offset + len)] = L'\0';                              \
    wcsncpy(expected, str_pre + offset, wcslen(str) - offset);                 \
    expected[wcslen(str) - offset] = L'\0';                                    \
    assert_eq_wcs(substr, expected);                                           \
                                                                               \
    free(str_pre);                                                             \
    free(str);                                                                 \
    free(substr);                                                              \
    free(expected)

// BUG: segfault
// Test(gap_buffer, insert) {
//     wchar_t *inserted;
//     wchar_t *str_pre, *str;
//     wchar_t *substr, *expected;
//     size_t offset, len;

//     // test_gb_insert(5, 3, L"42blahfoo\nbar\n");
//     // test_gb_insert(1, 8, L"kkkkjjjjhjkl");
//     for (size_t i = 0; i < 42; i++) {
//         test_gb_insert(9, 1, L"abcdefghijklmnopqrstuvwxyz1234567890\n");
//     }
// }
