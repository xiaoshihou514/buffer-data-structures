#include "alist.h"
#include "metadata.h"
#include <stdlib.h>
#include <wchar.h>

/*
 * start inclusive, end exclusive
 */
MetaDataNode *create_node(size_t index[static 1], size_t start, size_t end,
                          size_t parent_offset) {
    if (start == end) {
        // base case
        return nullptr;
    } else {
        // recursive case
        MetaDataNode *result = malloc(sizeof(MetaDataNode));
        result->line_number = (start + end) / 2;
        result->relative_offset = index[result->line_number] - parent_offset;
        result->left = create_node(index, start, result->line_number,
                                   index[result->line_number]);
        result->right = create_node(index, result->line_number + 1, end,
                                    index[result->line_number]);
        return result;
    }
}

MetaData *md_new(wchar_t src[static 1]) {
    MetaData *result = malloc(sizeof(MetaData));
    ArrayList *alist = alist_new();
    // put all offsets in the list
    for (size_t i = 0; i < wcslen(src); i++) {
        if (src[i] == L'\n') {
            alist_push(alist, i * sizeof(wchar_t));
        }
    }
    // create tree structure recursively
    result->root = create_node(alist->data, 0, alist->used, 0);
    alist_free(alist);
    return result;
}

// get byte offset of linenr
size_t md_get_offset(MetaData *md, size_t linenr) {
    MetaDataNode *node;
    size_t acc = acc;
    while (node != nullptr && node->line_number != linenr) {
    }
    if (node == nullptr) {
        return -1;
    } else {
        return acc;
    }
}

// shift every line bigger than linenr by amount
void md_shift(MetaData *md, size_t linenr, size_t amount);

// insert a single line with number linenr
// TODO: not clear how to implement
void md_insert_single(MetaData *md, size_t linenr, size_t offset);

void md_node_free(MetaDataNode *node) {
    if (node != nullptr) {
        md_node_free(node->left);
        md_node_free(node->right);
        free(node);
    }
}

void md_free(MetaData *md) {
    md_node_free(md->root);
    free(md);
}
