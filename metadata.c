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
size_t md_get_line_start(MetaData *md, size_t linenr) {
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
        if ((size_t)row_acc == linenr) {
            break;
        } else if ((size_t)row_acc < linenr) {
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
        if ((size_t)row_acc == linenr) {
            break;
        } else if ((size_t)row_acc < linenr) {
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
        if (child == node->left) {
            // child is left of node and we reached child via its right node
            if (!left) {
                child->relative_offset -= amount;
            }
            left = true;
        } else {
            // child is right of node and we reached child via its left node
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

size_t node_get_depth(MetaDataNode *node) {
    if (!node) {
        return 0;
    }
    size_t left = node_get_depth(node->left);
    size_t right = node_get_depth(node->right);
    return (left > right) ? left + 1 : right + 1;
}

// returns the new root node
MetaDataNode *rotate_left(MetaDataNode *a) {
    MetaDataNode *b = a->right;
    if (!b) {
        return a;
    }
    MetaDataNode *beta = b->left;

    // b would become the root, so we make its relatve params absolute
    b->relative_offset += a->relative_offset;
    b->relative_linenr += a->relative_linenr;
    // beta had parent b and now has parent a, we have to adjust that
    if (beta) {
        beta->relative_linenr += b->relative_linenr;
        beta->relative_offset += b->relative_offset;
        beta->relative_linenr -= a->relative_linenr;
        beta->relative_offset -= a->relative_offset;
    }
    // a is now relative
    a->relative_offset -= b->relative_offset;
    a->relative_linenr -= b->relative_linenr;

    // do the rotation
    a->right = beta;
    b->left = a;
    return b;
}

MetaDataNode *rotate_right(MetaDataNode *b) {
    MetaDataNode *a = b->left;
    if (!a) {
        return b;
    }
    MetaDataNode *beta = a->right;

    // a would become the root, so we make its relatve params absolute
    a->relative_offset += b->relative_offset;
    a->relative_linenr += b->relative_linenr;
    // beta had parent a and now has parent b, we have to adjust that
    if (beta) {
        beta->relative_linenr += a->relative_linenr;
        beta->relative_offset += a->relative_offset;
        beta->relative_linenr -= b->relative_linenr;
        beta->relative_offset -= b->relative_offset;
    }
    // b is now relative
    b->relative_offset -= a->relative_offset;
    b->relative_linenr -= a->relative_linenr;

    // do the rotation
    b->left = beta;
    a->right = b;
    return a;
}

void balance_ancestor_of_node(MetaDataNode *target, size_t node_left_depth,
                              size_t node_right_depth, bool initial_left,
                              MetaData *md) {
    size_t left_depth = node_left_depth;
    size_t right_depth = node_right_depth;
    bool left = initial_left;
    bool left_of_parent;
    MetaDataNode *node = target;
    MetaDataNode *parent;

    while (node->parent) {
        // in this loop we make `node` balanced
        // update depth lazily
        if (left) {
            left_depth =
                (left_depth > right_depth) ? left_depth + 1 : right_depth + 1;
            right_depth = node_get_depth(node->right) + 1;
        } else {
            right_depth =
                (left_depth > right_depth) ? left_depth + 1 : right_depth + 1;
            left_depth = node_get_depth(node->right) + 1;
        }

        // see which parent pointer we have to update
        left_of_parent = node->parent->left == node;
        parent = node->parent;
        if (left_depth > right_depth + 1) {
            // rotate right to fix this
            for (size_t i = 0; i < (left_depth - right_depth) / 2; i++) {
                node = rotate_right(node);
            }
            if (left_of_parent) {
                parent->left = node;
            } else {
                parent->right = node;
            }
            node->parent = parent;
        } else if (right_depth > left_depth + 1) {
            // rotate left instead
            for (size_t i = 0; i < (right_depth - left_depth) / 2; i++) {
                node = rotate_left(node);
            }
            if (left_of_parent) {
                parent->left = node;
            } else {
                parent->right = node;
            }
            node->parent = parent;
        }

        left = left_of_parent;
        node = parent;
    }

    // now we should have reached the root node, this is a special case since
    // the root node does not have a parent and we have to update md->root
    // directly, but the reset of the logic is basically copy pasting
    if (left) {
        left_depth =
            (left_depth > right_depth) ? left_depth + 1 : right_depth + 1;
        right_depth = node_get_depth(node->right) + 1;
    } else {
        right_depth =
            (left_depth > right_depth) ? left_depth + 1 : right_depth + 1;
        left_depth = node_get_depth(node->right) + 1;
    }
    if (left_depth > right_depth + 1) {
        // rotate right to fix this
        for (size_t i = 0; i < left_depth - right_depth / 2; i++) {
            node = rotate_right(node);
        }
        md->root = node;
    } else if (right_depth > left_depth + 1) {
        // rotate left instead
        for (size_t i = 0; i < right_depth - left_depth / 2; i++) {
            node = rotate_left(node);
        }
        md->root = node;
    }
}

void md_insert(MetaData *md, size_t linenr) {
    if (linenr > md->rows) {
        // invalid
        return;
    }
    // update count
    md->rows++;
    // if tree empty
    if (!md->root && linenr == 1) {
        md->root = malloc(sizeof(MetaDataNode));
        md->root->left = nullptr;
        md->root->right = nullptr;
        md->root->parent = nullptr;
        md->root->relative_linenr = 1;
        md->root->relative_offset = 0;
        return;
    }
    // if tree not empty
    MetaDataNode *target;
    MetaDataNode *node = md->root;
    ssize_t row_acc = 0;
    while (node) {
        row_acc += node->relative_linenr;
        if ((size_t)row_acc == linenr) {
            break;
        } else if ((size_t)row_acc > linenr) {
            node = node->left;
        } else {
            node = node->right;
        }
    }
    // save it for later
    target = node;
    MetaDataNode *child = node->left;
    if (child) {
        child->relative_linenr--;
    }
    bool left = true;
    // shift relative linenr
    while (node->parent) {
        child = node;
        node = node->parent;
        if (child == node->left) {
            // child is left of node and we reached child via its right node
            if (!left) {
                child->relative_linenr--;
            }
            left = true;
        } else {
            // child is right of node and we reached child via its left node
            if (left) {
                child->relative_linenr++;
            }
            left = false;
        }
    }
    if (left) {
        node->relative_linenr++;
    }

    // shift relative offset
    md_shift(md, linenr + 1, 1);

    // insert and rotate
    MetaDataNode *new = malloc(sizeof(MetaDataNode));
    // correct error

    if (target->left) {
        // fix error made in the earliler while loop
        target->left->relative_linenr++;
        target->left->relative_offset += 2;
    }

    // FIXME: relative offset
    new->relative_offset = -2;
    new->relative_linenr = -1;
    new->parent = target;
    // don' forget the original left node, we can't lose it!
    new->left = target->left;
    new->right = nullptr;

    // the new node is always put to the left of the previous one
    target->left = new;

    // balance every node that's an ancestor (and including) new
    balance_ancestor_of_node(new, node_get_depth(new->left), 0, true, md);
}

// delete the given line
void md_delete(MetaData *md, size_t linenr);

void md_node_free(MetaDataNode *node) {
    if (node) {
        md_node_free(node->left);
        md_node_free(node->right);
        free(node);
    }
}

void md_free(MetaData *md) {
    if (md) {
        md_node_free(md->root);
        free(md);
    }
}
