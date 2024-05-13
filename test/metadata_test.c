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
    alist_push(alist, 0);
    for (size_t i = 0; i < wcslen(src); i++) {
        if (src[i] == L'\n') {
            alist_push(alist, i * sizeof(wchar_t));
        }
    }

    alist_simple = alist_new();
    alist_push(alist_simple, 0);
    for (size_t i = 0; i < wcslen(src_simple); i++) {
        if (src_simple[i] == L'\n') {
            alist_push(alist_simple, i * sizeof(wchar_t));
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
    cr_assert_eq(src_simple[root_offset / sizeof(wchar_t)], L'\n');
    cr_assert_eq(src_simple[(root_offset + left_offset) / sizeof(wchar_t)],
                 L'\n');
    cr_assert_eq(src_simple[(root_offset + right_offset) / sizeof(wchar_t)],
                 L'\n');

    cr_assert_eq(src_simple[(root_offset + right_offset +
                             root->right->left->relative_offset) /
                            sizeof(wchar_t)],
                 L'\n');
    // note that line 1's line break does not have a real \n
    cr_assert_eq(root_offset + left_offset + root->left->left->relative_offset,
                 0);
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

#define assert_insert_simple                                                   \
    MetaDataNode *root = md_simple->root;                                      \
    assert_eq_i64(root->relative_linenr, 4);                                   \
    assert_eq_i64(root->left->relative_linenr, -2);                            \
    assert_eq_i64(root->left->left->relative_linenr, -1);                      \
    assert_eq_i64(root->left->right->relative_linenr, 1);                      \
    cr_assert(eq(ptr, root->left->left->left, nullptr));                       \
    assert_eq_i64(root->right->relative_linenr, 2);                            \
    assert_eq_i64(root->right->left->relative_linenr, -1)

Test(metadata, insert_simple_linenr) {
    md_insert(md_simple, 2);
    /* before it got balanced, it should give:
     *          4               4
     *         / \             / \
     *        3   6          -1   2
     *       /   /     or    /   /
     *      2   5          -1   -1
     *     /               /
     *    1              -1
     */
    // assert_eq_i64(root->relative_linenr, 4);
    // assert_eq_i64(root->left->relative_linenr, -1);
    // assert_eq_i64(root->left->left->relative_linenr, -1);
    // assert_eq_i64(root->left->left->left->relative_linenr, -1);
    // assert_eq_i64(root->right->relative_linenr, 2);
    // assert_eq_i64(root->right->left->relative_linenr, -1);

    /* after balancing, it should give
     *          4             4
     *         / \           / \
     *        2   6    or  -2   2
     *       / \ /         / \ /
     *      1  3 5       -1  1 -1
     */

    assert_insert_simple;
}

// the following should give the same results
Test(metadata, insert_simple_tail) {
    md_insert(md_simple, 1);
    assert_insert_simple;
}

Test(metadata, insert_simple_root) {
    md_insert(md_simple, 3);
    assert_insert_simple;
}

Test(metadata, insert_simple_twice) {
    md_insert(md_simple, 4);
    MetaDataNode *root = md_simple->root;

    /*        3              3
     *       / \            / \
     *      2   5    or   -1   2
     *     /   / \        /   / \
     *    1   4   6     -1  -1   1
     */
    assert_eq_i64(root->relative_linenr, 3);
    assert_eq_i64(root->left->relative_linenr, -1);
    assert_eq_i64(root->left->left->relative_linenr, -1);
    cr_assert(eq(ptr, root->left->left->left, nullptr));
    assert_eq_i64(root->right->relative_linenr, 2);
    assert_eq_i64(root->right->left->relative_linenr, -1);
    assert_eq_i64(root->right->right->relative_linenr, 1);

    md_insert(md_simple, 6);
    /*        3              3
     *       / \            / \
     *      2   5    or   -1   2
     *     /   / \        /   / \
     *    1   4   7     -1  -1   2
     *           /              /
     *          6             -1
     */
    assert_eq_i64(root->relative_linenr, 3); // is 4
    assert_eq_i64(root->left->relative_linenr, -1);
    assert_eq_i64(root->left->left->relative_linenr, -1);
    cr_assert(eq(ptr, root->left->left->left, nullptr));
    assert_eq_i64(root->right->relative_linenr, 2);
    assert_eq_i64(root->right->left->relative_linenr, -1);
    assert_eq_i64(root->right->right->relative_linenr, 2); // is 1
    assert_eq_i64(root->right->right->left->relative_linenr, -1);
    cr_assert(eq(ptr, root->right->right->right, nullptr));
}

Test(metadata, insert_simple_offset) {
    md_insert(md_simple, 2);
    MetaDataNode *root = md_simple->root;
    /*          4         PRE: linenr    offset     POST: linenr    offset
     *         / \                1    |   0                 1    |   0
     *        2   6               2    |   564  ──────┐      2    |   563
     *       / \ /                3    |   1156 ─────┐└───>  3    |   565
     *      1  3 5                4    |   1724 ────┐└────>  4    |   1157
     *                            5    |   2320 ───┐└─────>  5    |   1725
     *                                             └──────>  6    |   2321
     */
    // test for relative values
    assert_eq_i64(root->relative_offset, 1157);
    assert_eq_i64(root->left->relative_offset, 563 - 1157);
    assert_eq_i64(root->left->left->relative_offset, 0 - 563);
    assert_eq_i64(root->left->right->relative_offset, 565 - 563);
    assert_eq_i64(root->right->relative_offset, 2321 - 1157);
    assert_eq_i64(root->right->left->relative_offset, 1725 - 2321);

    /* test for actually getting these values */
    assert_eq_i64(md_get_line_start(md_simple, 1), 0);
    /* the second line is inserted right after the first line ends, i.e.
     * md_get_offset(md, 2) - 1
     * that corresponds to the user pressing <cr> at the end on line 1
     */
    assert_eq_i64(md_get_line_start(md_simple, 2), 563);
    assert_eq_i64(md_get_line_start(md_simple, 3), 565);
    assert_eq_i64(md_get_line_start(md_simple, 4), 1157);
    assert_eq_i64(md_get_line_start(md_simple, 5), 1725);
    assert_eq_i64(md_get_line_start(md_simple, 6), 2321);
}

Test(metadata, insert_complex_offset) {
    for (size_t i = 0; i < alist->used; i++) {
        md = md_new(src);
        md_insert(md, i + 1);
        for (size_t j = 0; j < i; j++) {
            // [1..i) shouldn't change
            assert_eq_i64(md_get_line_start(md, j + 1), alist->data[j]);
        }
        // i should be original offset - 1
        assert_eq_i64(md_get_line_start(md, i + 1), alist->data[i] - 1);
        for (size_t k = i + 1; k <= alist->used; k++) {
            // (j..] should increment by 1, and be offsetted by 1
            assert_eq_i64(md_get_line_start(md, k + 1), alist->data[k - 1] + 1);
        }
    }
}

Test(metadata, delete_simple) {
    md_delete_line_break(md_simple, 2);
    MetaDataNode *root = md_simple->root;
    /*
     * original:                  after:
     *      3             3          2            2
     *     / \           / \        / \          / \
     *    2   5   or   -1   2      1   4   or  -1   2
     *   /   /         /   /          /            /
     *  1   4        -1   -1         3           -1
     *
     * change in relative_offset:
     *        1156                   1155                  1155
     *       /    \                 /    \                /    \
     *    -592   1164    --->    -592   1164   --->    -1155  1164
     *     /      /               /      /                     /
     *   -564   -596            -563   -596                  -596
     *
     */
    assert_eq_i64(root->relative_linenr, 2);
    assert_eq_i64(root->left->relative_linenr, -1);
    assert_eq_i64(root->right->relative_linenr, 2);
    assert_eq_i64(root->right->left->relative_linenr, -1);

    assert_eq_i64(root->relative_offset, 1155);
    assert_eq_i64(root->left->relative_offset, -1155);
    assert_eq_i64(root->right->relative_offset, 1164);
    assert_eq_i64(root->right->left->relative_offset, -596);

    md_simple = md_new(src_simple);
    root = md_simple->root;
    md_delete_line_break(md_simple, 3);
    /*
     * change in relative_linenr:
     *      3               3               3                        3
     *     / \    decr     / \    swap     / \    remove single     / \
     *    2   5   --->    2   4   --->    2   4   ------------>    2   4
     *   /   /           /   /           /   /                    /
     *  1   4           1   3           1   3                    1
     *
     *      3               3               3                        3
     *     / \    decr     / \    swap     / \    remove single     / \
     *   -1   2   --->   -1   1   --->   -1   1   ------------>   -1   1
     *   /   /           /   /           /   /                    /
     * -1   -1         -1  -1          -1   3                   -1
     *
     * change in relative_offset:
     *        1156                   1156                  1723
     *       /    \                 /    \                /    \
     *    -592   1164    --->    -592   1163   --->    -1159  596
     *     /      /               /      /              /
     *   -564   -596            -564   -596           -564
     */
    assert_eq_i64(root->relative_linenr, 3);
    assert_eq_i64(root->left->relative_linenr, -1);
    assert_eq_i64(root->left->left->relative_linenr, -1);
    assert_eq_i64(root->right->relative_linenr, 1);

    assert_eq_i64(root->relative_offset, 1723);
    assert_eq_i64(root->left->relative_offset, -1159);
    assert_eq_i64(root->left->left->relative_offset, -564);
    assert_eq_i64(root->right->relative_offset, 596);

    md_simple = md_new(src_simple);
    root = md_simple->root;
    md_delete_line_break(md_simple, 1);
    /* clang-format off
     *
     * change in linenr:
     *      3               2                       2             2
     *     / \    decr     / \    remove single    / \           / \
     *    2   5   --->    1   4   ------------>   1   4    or  -1   2
     *   /   /           /   /                       /             /
     *  1   4           1   3                       3             -1
     *
     * change in relative_offset:
     *        1156                   1155
     *       /    \                 /    \
     *    -592   1164    --->    -592   1164
     *     /      /                      /
     *   -564   -596                   -596
     *
     * clang-format on
     */
    assert_eq_i64(root->relative_linenr, 2);
    assert_eq_i64(root->left->relative_linenr, -1);
    assert_eq_i64(root->right->relative_linenr, 2);
    assert_eq_i64(root->right->left->relative_linenr, -1);

    assert_eq_i64(root->relative_offset, 1155);
    assert_eq_i64(root->left->relative_offset, -592);
    assert_eq_i64(root->right->relative_offset, 1164);
    assert_eq_i64(root->right->left->relative_offset, -596);
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
        assert_eq_i64(md_get_line_start(md, alist->used), -1);
    }
}
