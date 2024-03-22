#include <stddef.h>

struct MetaDataNode {
    size_t line_number;
    // so data[relative_offset] should be '\n'
    size_t relative_offset;
    struct MetaDataNode *left;
    struct MetaDataNode *right;
};

typedef struct MetaDataNode MetaDataNode;

typedef struct {
    MetaDataNode *root;
} MetaData;

MetaData *md_new(wchar_t src[static 1]);

// get byte offset of linenr
size_t md_get_offset(MetaData *md, size_t linenr);

// shift every line bigger than linenr by amount
void md_shift(MetaData *md, size_t linenr, size_t amount);

// insert a single line with number linenr
// TODO: not clear how to implement
void md_insert_single(MetaData *md, size_t linenr, size_t offset);

void md_free(MetaData md[static 1]);
