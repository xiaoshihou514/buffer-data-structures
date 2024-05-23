#include "../alist.h"
#include "../metadata.h"
#include "criterion/logging.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <wchar.h>

MetaData *md;
MetaData *md_simple;
ArrayList *alist;
ArrayList *alist_simple;
wchar_t *src_simple =
    L"A gap buffer in computer science is a dynamic array that allows "
    L"efficient insertion and deletion operations clustered near the same "
    L"location.\n Gap buffers are especially common in text editors, where "
    L"most changes to the text occur at or near the current location of the "
    L"cursor. The text is\n stored in a large buffer in two contiguous "
    L"segments, with a gap between them for inserting new text. Moving the "
    L"cursor involves copying text\n from one side of the gap to the other "
    L"(sometimes copying is delayed until the next operation that changes the "
    L"text). Insertion adds new text at the\n end of the first segment; "
    L"deletion deletes it.";
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

void setup(void) {
    md = md_new(src);
    md_simple = md_new(src_simple);

    alist = alist_new();
    alist_push(alist, -1);
    for (size_t i = 0; i < wcslen(src); i++) {
        if (src[i] == L'\n') {
            alist_push(alist, i);
        }
    }

    alist_simple = alist_new();
    alist_push(alist_simple, -1);
    for (size_t i = 0; i < wcslen(src_simple); i++) {
        if (src_simple[i] == L'\n') {
            alist_push(alist_simple, i);
        }
    }
}

void teardown(void) {
    md_free(md);
    md_free(md_simple);
    alist_free(alist);
    alist_free(alist_simple);
}

#define assert_eq_sz(...) cr_assert(eq(sz, ##__VA_ARGS__))
#define assert_eq_i64(...) cr_assert(eq(i64, ##__VA_ARGS__))

#define md_get_works                                                           \
    for (size_t i = 0; i < alist->used; i++) {                                 \
        cr_assert(eq(sz, md_get_line_start(md, i + 1), alist->data[i]));       \
    }

TestSuite(metadata, .init = setup, .fini = teardown);

Test(metadata, new_simple) {
    /* should give
     *          3                      3
     *        /   \                  /   \
     *     2         5     or     -1        2
     *   /   \     /   \         /   \     /   \
     *  1   null  4   null    -1    null -1   null
     */

    // check line numbers
    MetaDataNode *root = md_simple->root;
    assert_eq_sz(3, root->relative_linenr);
    assert_eq_sz(-1, root->left->relative_linenr);
    assert_eq_sz(2, root->right->relative_linenr);
    assert_eq_sz(-1, root->left->left->relative_linenr);
    assert_eq_sz(-1, root->right->left->relative_linenr);

    cr_assert_null(root->left->right);
    cr_assert_null(root->right->right);

    // check parent is indeed parent
    cr_assert(eq(ptr, root, root->left->parent));
    cr_assert(eq(ptr, root, root->right->parent));
    cr_assert(eq(ptr, root->left, root->left->left->parent));
    cr_assert(eq(ptr, root->right, root->right->left->parent));

    // check they are indeed line breaks
    ssize_t root_offset = root->relative_offset;
    ssize_t left_offset = root->left->relative_offset;
    ssize_t right_offset = root->right->relative_offset;
    cr_assert_eq(src_simple[root_offset], L'\n');
    cr_assert_eq(src_simple[(root_offset + left_offset)], L'\n');
    cr_assert_eq(src_simple[(root_offset + right_offset)], L'\n');

    cr_assert_eq(src_simple[(root_offset + right_offset +
                             root->right->left->relative_offset)],
                 L'\n');
    // note that line 1's line break does not have a real \n
    cr_assert_eq(root_offset + left_offset + root->left->left->relative_offset,
                 -1);
}

Test(metadata, get){md_get_works}

Test(metadata, shift_offset) {
    for (size_t i = 0; i < alist_simple->used; i++) {
        md_simple = md_new(src_simple);
        md_shift_offset(md_simple, i + 1, 42, nullptr);

        // everything before i should stay the same
        for (size_t j = 0; j < i; j++) {
            assert_eq_sz(md_get_line_start(md_simple, j + 1),
                         alist_simple->data[j]);
        }

        // ...and everything after should be incremented by 42
        for (size_t k = i; k < alist_simple->used; k++) {
            assert_eq_sz(md_get_line_start(md_simple, k + 1),
                         alist_simple->data[k] + 42);
        }
    }

    // same test for a more complicated structure,and this time decrementing
    for (size_t i = 0; i < alist->used; i++) {
        md = md_new(src);
        md_shift_offset(md, i + 1, -42, nullptr);

        // everything before i should stay the same
        for (size_t j = 0; j < i; j++) {
            assert_eq_sz(md_get_line_start(md, j + 1), alist->data[j]);
        }

        // ...and everything after should be decremented by 42
        for (size_t k = i; k < alist->used; k++) {
            assert_eq_sz(md_get_line_start(md, k + 1), alist->data[k] - 42);
        }
    }
}

Test(metadata, shift_offset_tail) {
    md_shift_offset(md_simple, alist_simple->used, 42, nullptr);
    for (size_t i = 0; i < alist_simple->used - 1; i++) {
        assert_eq_sz(md_get_line_start(md_simple, i + 1),
                     alist_simple->data[i]);
    }
    assert_eq_sz(md_get_line_start(md_simple, alist_simple->used),
                 alist_simple->data[alist_simple->used - 1] + 42);
}

Test(metadata, rotate_simple) {
    md_simple->root = rotate_left(md_simple->root);
    /* should give
     *        5                 5
     *       /                 /
     *      3                 -2
     *     / \        or     / \
     *    2   4             -1  1
     *   /                 /
     *  1                 -1
     */
    MetaDataNode *root = md_simple->root;
    assert_eq_sz(5, root->relative_linenr);
    cr_assert_null(root->right);
    assert_eq_sz(-2, root->left->relative_linenr);
    assert_eq_sz(1, root->left->right->relative_linenr);
    assert_eq_sz(-1, root->left->left->relative_linenr);
    assert_eq_sz(-1, root->left->left->left->relative_linenr);

    md_simple = md_new(src_simple);
    md_simple->root = rotate_right(md_simple->root);
    /* should give
     *    2                2
     *   / \              / \
     *  1   3       or  -1   1
     *       \                \
     *        5                2
     *       /                /
     *      4                -1
     */
    root = md_simple->root;
    assert_eq_sz(2, root->relative_linenr);
    assert_eq_sz(-1, root->left->relative_linenr);
    assert_eq_sz(1, root->right->relative_linenr);
    assert_eq_sz(2, root->right->right->relative_linenr);
    assert_eq_sz(-1, root->right->right->left->relative_linenr);
}

Test(metadata, rotate_complex_left) {
    md->root = rotate_left(md->root);
    md_get_works
}

Test(metadata, rotate_complex_right) {
    md->root = rotate_left(md->root);
    md_get_works
}

Test(metadata, delete_any_complex) {
    for (size_t i = 1; i < alist->used; i++) {
        md = md_new(src);
        md_delete_line_break(md, i);
        for (size_t j = 1; j < i; j++) {
            // [1..i) shouldn't change
            assert_eq_i64(md_get_line_start(md, j), alist->data[j - 1]);
        }
        for (size_t k = i; k < alist->used; k++) {
            // forall k in (i..]
            // md[k] should be md_old[k+1]-1
            assert_eq_i64(md_get_line_start(md, k), alist->data[k] - 1);
        }
        // the last one shouldn't exist anymore
    }
}

// BUG: breaks for i=20
Test(metadata, insert_offset) {
    for (size_t i = 1; i <= alist->used; i++) {
        md = md_new(src);
        md_insert(md, i, 0);
        cr_log_warn("inserted on line %zu", i);
        for (size_t j = 0; j < i; j++) {
            // [1..i+1) shouldn't change
            cr_log_warn("line %zu is %zu, should be %zu", j + 1,
                        md_get_line_start(md, j + 1), alist->data[j]);
            assert_eq_i64(md_get_line_start(md, j + 1), alist->data[j]);
        }
        // i+2 is the inserted entry
        cr_log_warn("line %zu is %zu, should be %zu", i + 1,
                    md_get_line_start(md, i + 1), alist->data[i - 1] + 1);
        assert_eq_i64(md_get_line_start(md, i + 1), alist->data[i - 1] + 1);
        for (size_t k = i + 1; k <= alist->used; k++) {
            // (j..] should increment by 1, and be offsetted by 1
            cr_log_warn("line %zu is %zu, should be %zu", k + 1,
                        md_get_line_start(md, k + 1), alist->data[k - 1] + 1);
            assert_eq_i64(md_get_line_start(md, k + 1), alist->data[k - 1] + 1);
        }
    }
}
