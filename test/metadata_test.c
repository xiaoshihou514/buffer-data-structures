#include "../metadata.h"
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

Test(metadata, new) { cr_expect(md != nullptr); }
