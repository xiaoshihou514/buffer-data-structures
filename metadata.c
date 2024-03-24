#include "alist.h"
#include "metadata.h"
#include "sys/types.h"
#include <stdlib.h>
#include <wchar.h>

/*
 * start inclusive, end exclusive
 */
MetaDataNode *create_node(ssize_t index[static 1], size_t start, size_t end,
                          size_t parent_offset, MetaDataNode *parent,
                          ssize_t parent_index) {
    if (start == end) {
        // base case
        return nullptr;
    } else {
        // recursive case
        MetaDataNode *result = malloc(sizeof(MetaDataNode));
        size_t idx = (start + end) / 2;
        size_t idx_offset = index[idx];

        // stored line number is one indexed
        result->relative_linenr = idx + 1 - parent_index;
        result->relative_offset = idx_offset - parent_offset;
        result->parent = parent;
        result->left =
            create_node(index, start, idx, idx_offset, result, idx + 1);
        result->right =
            create_node(index, idx + 1, end, idx_offset, result, idx + 1);
        return result;
    }
}

MetaData *md_new(wchar_t src[static 1]) {
    MetaData *result = malloc(sizeof(MetaData));
    ArrayList *alist = alist_new();
    // put all offsets in the list
    wchar_t *idx = src;
    while (idx != (void *)0x4) {
        alist_push(alist, (ssize_t)(idx - src - 1) * sizeof(wchar_t));
        idx = wcsstr(idx, L"\n") + 1;
    }
    // fix error, it's -4 beforehand
    alist->data[0] += 4;
    // create tree structure recursively
    result->root = create_node(alist->data, 0, alist->used, 0, nullptr, 0);
    result->rows = alist->used;
    alist_free(alist);
    return result;
}

// get byte offset of linenr
size_t md_get_offset(MetaData *md, size_t linenr) {
    if (linenr > md->rows) {
        // invalid
        return -1;
    }
    MetaDataNode *node = md->root;
    ssize_t row_acc = 0;
    ssize_t acc = 0;
    while (node) {
        acc += node->relative_offset;
        row_acc += node->relative_linenr;
        if (row_acc == linenr) {
            break;
        } else if (row_acc < linenr) {
            node = node->right;
        } else {
            node = node->left;
        }
    }
    // it's safe to do so since we can guarantee it's positive
    return (size_t)acc;
}

// shift every line bigger or equal to linenr by amount
void md_shift(MetaData *md, size_t linenr, ssize_t amount) {
    if (linenr > md->rows) {
        // invalid
        return;
    }
    MetaDataNode *node = md->root;
    ssize_t row_acc = 0;
    while (node) {
        row_acc += node->relative_linenr;
        if (row_acc == linenr) {
            break;
        } else if (row_acc < linenr) {
            node = node->right;
        } else {
            node = node->left;
        }
    }
    MetaDataNode *child = node->left;
    if (child) {
        child->relative_offset -= amount;
    }
    bool left = true;
    while (node->parent) {
        child = node;
        node = node->parent;
        // child is left of node and we reached child via its right node
        if (child == node->left) {
            if (!left) {
                child->relative_offset -= amount;
            }
            left = true;
        }
        // child is right of node and we reached child via its left node
        if (child == node->right) {
            if (left) {
                child->relative_offset += amount;
            }
            left = false;
        }
    }
    if (left) {
        node->relative_offset += amount;
    }
}

// TODO: come back to this
void md_insert(MetaData *md, size_t linenr) {
    // if tree empty
    if (md->root == nullptr && linenr == 1) {
        md->root = malloc(sizeof(MetaDataNode));
        md->root->left = nullptr;
        md->root->right = nullptr;
        md->root->parent = nullptr;
        md->root->relative_linenr = 1;
        md->root->relative_offset = 0;
        md->rows++;
        return;
    }
    // if tree not empty
    MetaDataNode *parent;
    MetaDataNode *current = md->root;
    ssize_t row_acc = 0;
    while (current) {
        row_acc += current->relative_linenr;
    }
}

// delete the given line
void md_delete(MetaData *md, size_t linenr);

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
