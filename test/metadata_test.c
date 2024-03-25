#include "../alist.h"
#include "../metadata.h"
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

#define md_get_works                                                           \
    for (size_t i = 0; i < alist->used; i++) {                                 \
        cr_assert(eq(sz, md_get_offset(md, i + 1), alist->data[i]));           \
    }

TestSuite(metadata, .init = setup, .fini = teardown);

Test(metadata, new) {
    /* should give
     *          3                      3
     *        /   \                  /   \
     *     2         5     or     -1        2
     *   /   \     /   \        /   \     /   \
     *  1   null  4   null     -1   null  -1   null
     */

    // check line numbers
    MetaDataNode *root = md_simple->root;
    cr_assert(eq(sz, 3, root->relative_linenr));
    cr_assert(eq(sz, -1, root->left->relative_linenr));
    cr_assert(eq(sz, 2, root->right->relative_linenr));
    cr_assert(eq(sz, -1, root->left->left->relative_linenr));
    cr_assert(eq(sz, -1, root->right->left->relative_linenr));

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
    cr_assert(eq(sz, L'\n', src_simple[root_offset / sizeof(wchar_t)]));
    cr_assert(eq(sz, L'\n',
                 src_simple[(root_offset + left_offset) / sizeof(wchar_t)]));
    cr_assert(eq(sz, L'\n',
                 src_simple[(root_offset + right_offset) / sizeof(wchar_t)]));

    cr_assert(eq(sz, L'\n',
                 src_simple[(root_offset + right_offset +
                             root->right->left->relative_offset) /
                            sizeof(wchar_t)]));
    // note that line 1's line break does not have a real \n
    cr_assert(eq(
        sz, 0, root_offset + left_offset + root->left->left->relative_offset));
}

Test(metadata, get) {
    md_get_works for (size_t i = alist->used; i < 42; i++) {
        cr_assert(eq(sz, md_get_offset(md, i + 1), -1));
    }
}

Test(metadata, shift) {
    for (size_t i = 0; i < alist_simple->used; i++) {
        md_simple = md_new(src_simple);
        md_shift(md_simple, i + 1, 42);

        // everything before i should stay the same
        for (size_t j = 0; j < i; j++) {
            cr_assert(
                eq(sz, md_get_offset(md_simple, j + 1), alist_simple->data[j]));
        }

        // ...and everything after should be incremented by 42
        for (size_t k = i; k < alist_simple->used; k++) {
            cr_assert(eq(sz, md_get_offset(md_simple, k + 1),
                         alist_simple->data[k] + 42));
        }
    }

    // same test for a more complicated structure,and this time decrementing
    for (size_t i = 0; i < alist->used; i++) {
        md = md_new(src);
        md_shift(md, i + 1, -42);

        // everything before i should stay the same
        for (size_t j = 0; j < i; j++) {
            cr_assert(eq(sz, md_get_offset(md, j + 1), alist->data[j]));
        }

        // ...and everything after should be decremented by 42
        for (size_t k = i; k < alist->used; k++) {
            cr_assert(eq(sz, md_get_offset(md, k + 1), alist->data[k] - 42));
        }
    }
}

Test(metadata, rotate_simple) {
    md_simple->root = rotate_left(md_simple->root);
    /* should give
     *        5                 5
     *       /                 /
     *      3                 -2
     *     / \        or     / \
     *    2   4             -1   1
     *   /                 /
     *  1                 -1
     */
    MetaDataNode *root = md_simple->root;
    cr_assert(eq(sz, 5, root->relative_linenr));
    cr_assert_null(root->right);
    cr_assert(eq(sz, -2, root->left->relative_linenr));
    cr_assert(eq(sz, 1, root->left->right->relative_linenr));
    cr_assert(eq(sz, -1, root->left->left->relative_linenr));
    cr_assert(eq(sz, -1, root->left->left->left->relative_linenr));

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
    cr_assert(eq(sz, 2, root->relative_linenr));
    cr_assert(eq(sz, -1, root->left->relative_linenr));
    cr_assert(eq(sz, 1, root->right->relative_linenr));
    cr_assert(eq(sz, 2, root->right->right->relative_linenr));
    cr_assert(eq(sz, -1, root->right->right->left->relative_linenr));
}

Test(metadata, rotate_complex_left) {
    md->root = rotate_left(md->root);
    md_get_works
}

Test(metadata, rotate_complex_right) {
    md->root = rotate_left(md->root);
    md_get_works
}

Test(metadata, insert_simple) {
    md_insert(md_simple, 2);
    MetaDataNode *root = md_simple->root;
    /* if this is not balanced, it should give:
     *          4               4
     *         / \             / \
     *        3   6          -1   2
     *       /   /     or    /   /
     *      2   5          -1   -1
     *     /               /
     *    1              -1
     */
    // cr_assert(eq(i64, root->relative_linenr, 4));
    // cr_assert(eq(i64, root->left->relative_linenr, -1));
    // cr_assert(eq(i64, root->left->left->relative_linenr, -1));
    // cr_assert(eq(i64, root->left->left->left->relative_linenr, -1));
    // cr_assert(eq(i64, root->right->relative_linenr, 2));
    // cr_assert(eq(i64, root->right->left->relative_linenr, -1));

    /* after balancing, it should give
     *          4             4
     *         / \           / \
     *        2   6    or  -2   2
     *       / \ /         / \ /
     *      1  3 5       -1  1 -1
     */
    cr_assert(eq(i64, root->relative_linenr, 4));

    cr_assert(eq(i64, root->left->relative_linenr, -2));
    cr_assert(eq(i64, root->left->left->relative_linenr, -1));
    cr_assert(eq(i64, root->left->right->relative_linenr, 1));
    cr_assert(eq(ptr, root->left->left->left, nullptr));

    cr_assert(eq(i64, root->right->relative_linenr, 2));
    cr_assert(eq(i64, root->right->left->relative_linenr, -1));
}
