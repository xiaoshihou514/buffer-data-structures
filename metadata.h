#include <stddef.h>
#include <sys/types.h>

struct MetaDataNode {
    ssize_t relative_linenr;
    // byte offset of linenr in the gap buffer (does not take account of gap
    // position)
    ssize_t relative_offset;
    struct MetaDataNode *left;
    struct MetaDataNode *right;
    struct MetaDataNode *parent;
};

typedef struct MetaDataNode MetaDataNode;

typedef struct {
    MetaDataNode *root;
    size_t rows;
} MetaData;

MetaData *md_new(wchar_t src[static 1]);

/*
 * get byte offset of linenr
 */
size_t md_get_offset(MetaData *md, size_t linenr);

/*
 * shift every line bigger than linenr by amount
 */
void md_shift(MetaData *md, size_t linenr, ssize_t amount);

/*
 * insert a single line right after the current offset of linenr
 * e.g. for a ten line document, inserting at 7 would shift the current 7,8,9,10
 * offset by 1 and make them 8,9,10,100, this function also shifts the
 * nodes after for you
 */
void md_insert(MetaData *md, size_t linenr);

/*
 * delete the given line
 */
void md_delete(MetaData *md, size_t linenr);

/*
 * helper methods
 */
MetaDataNode *rotate_left(MetaDataNode *a);
MetaDataNode *rotate_right(MetaDataNode *a);

void md_free(MetaData *md);
