#include "../alist.h"
#include "../metadata.h"
#include "criterion/assert.h"
#include "criterion/internal/assert.h"
#include "criterion/logging.h"
#include <criterion/criterion.h>
#include <wchar.h>

MetaData *md;
wchar_t *src =
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

void new_metadata(void) { md = md_new(src); }

void free_metadata(void) { md_free(md); }

TestSuite(metadata, .init = new_metadata, .fini = free_metadata);

Test(metadata, new) {
    ArrayList *alist = alist_new();
    // put all offsets in the list
    for (size_t i = 0; i < wcslen(src); i++) {
        if (src[i] == L'\n') {
            alist_push(alist, i * sizeof(wchar_t));
        }
    }

    // root node is 2
    cr_expect_eq(md->root->line_number, 2);
    cr_expect_eq(src[md->root->relative_offset / sizeof(wchar_t)], L'\n');
    cr_expect_eq(md->root->relative_offset, alist->data[2]);

    // left is 1
    cr_expect_eq(md->root->left->relative_offset + md->root->relative_offset,
                 alist->data[1]);
    cr_expect_eq(
        src[(md->root->left->relative_offset + md->root->relative_offset) /
            sizeof(wchar_t)],
        L'\n');

    // right is 3
    cr_expect_eq(md->root->right->relative_offset + md->root->relative_offset,
                 alist->data[3]);
    cr_expect_eq(
        src[(md->root->right->relative_offset + md->root->relative_offset) /
            sizeof(wchar_t)],
        L'\n');

    // left of left is 0
    cr_expect_eq(md->root->left->left->line_number, 0);

    // all others are null
    cr_expect_null(md->root->left->right, "no more children on the right");
    cr_expect_null(md->root->right->left, "no children on the right");
    cr_expect_null(md->root->right->right, "no children on the right");
}
