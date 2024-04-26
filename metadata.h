#include <stddef.h>
#include <sys/types.h>

struct MetaDataNode {
    ssize_t relative_linenr;
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
 * get byte offset of the position where the `linenr`-th line ends
 * gap_buffer[r] = L'\n' (roughly)
 */
size_t md_get_line_start(MetaData *md, size_t linenr);

/*
 * shift every line bigger or equal to linenr by amount
 */
void md_shift_offset(MetaData *md, size_t linenr, ssize_t amount,
                     MetaDataNode *needle);

/*
 * insert a single line right after the current offset of linenr
 * e.g. for a 10 line document, inserting at 7 would shift the current
 * 7,8,9,10 offset by 1 and make them 8,9,10,11, this function also shifts
 * the nodes after for you
 */
void md_insert(MetaData *md, size_t linenr);

/*
 * delete the given "linenr"-th line break
 * e.g. for a 10 line document, deleting at 7 would shift 8,9,10 to be
 * 7,8,9, and shift their relative offset by -1
 */
void md_delete_line_break(MetaData *md, size_t linenr);

/*
 * helper methods
 */
MetaDataNode *rotate_left(MetaDataNode *a);
MetaDataNode *rotate_right(MetaDataNode *a);
void remove_single_node(MetaData *md, MetaDataNode *node);
void md_node_mov(MetaDataNode *to, MetaDataNode *from);

void md_free(MetaData *md);
