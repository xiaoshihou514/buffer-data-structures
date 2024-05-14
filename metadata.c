#include "alist.h"
#include "criterion/logging.h"
#include "metadata.h"
#include "sys/types.h"
#include <stdlib.h>
#include <wchar.h>

/* node helper methods */
// start inclusive, end exclusive
MetaDataNode *node_create(ssize_t index[static 1], size_t start, size_t end,
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
            node_create(index, start, idx, idx_offset, result, idx + 1);
        result->right =
            node_create(index, idx + 1, end, idx_offset, result, idx + 1);
        return result;
    }
}

MetaDataNode *node_seek(MetaData *md, size_t linenr) {
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
    return node;
}

size_t node_get_depth(MetaDataNode *node) {
    if (!node) {
        return 0;
    }
    size_t left = node_get_depth(node->left);
    size_t right = node_get_depth(node->right);
    return (left > right) ? left + 1 : right + 1;
}

/* md helper methods */
// shift every line bigger or equal to linenr by amount
void md_shift_offset(MetaData *md, size_t linenr, ssize_t amount,
                     MetaDataNode *needle) {
    if (linenr > md->rows) {
        cr_log_error("md_shift_offset: invalid line number");
        return;
    }
    MetaDataNode *node = needle ? needle : node_seek(md, linenr);
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

/*
 * shift linenr of every node larger or equal then the given node by amount
 */
void md_shift_linenr(MetaData *md, size_t linenr, ssize_t amount,
                     MetaDataNode *needle) {
    needle = needle ? needle : node_seek(md, linenr);
    MetaDataNode *child = needle->left;
    if (child) {
        child->relative_linenr -= amount;
    }
    bool left = true;
    while (needle->parent) {
        child = needle;
        needle = needle->parent;
        if (child == needle->left) {
            // child is left of needle and we reached child via its right node
            if (!left) {
                child->relative_linenr -= amount;
            }
            left = true;
        } else {
            // child is right of needle and we reached child via its left node
            if (left) {
                child->relative_linenr += amount;
            }
            left = false;
        }
    }
    if (left) {
        needle->relative_linenr += amount;
    }
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

// PRE: the current node will be non-null but at least one of its subtrees will
// be null
#define update(parent_direction, child_direction)                              \
    parent->parent_direction = node->child_direction;                          \
    if (node->child_direction) {                                               \
        node->child_direction->parent = parent;                                \
        node->child_direction->relative_offset += node->relative_offset;       \
    }

void remove_single_node(MetaData *md, MetaDataNode *node) {
    MetaDataNode *parent = node->parent;
    if (parent) {
        if (node == parent->left) {
            if (!node->left) {
                update(left, right);
            } else {
                update(left, left);
            }
        } else {
            if (!node->left) {
                update(right, right);
            } else {
                update(right, left);
            }
        }
    } else {
        if (!node->left) {
            md->root = node->right;
            if (node->right) {
                node->right->parent = nullptr;
            }
        } else {
            md->root = node->left;
            if (node->left) {
                node->left->parent = nullptr;
            }
        }
    }
    free(node);
}

// PRE: to is a parent of from
// POST: from = to = safe_swapped(from)
void md_node_mov(MetaDataNode *to, MetaDataNode *from) {
    // this is non-trivial because to do a safe swap we have to alter the
    // relative stuff along the way
    ssize_t original_relative_linenr = to->relative_linenr;
    ssize_t original_relative_offset = to->relative_offset;
    MetaDataNode *temp = from->parent;

    // add up along the way
    while (temp != to) {
        from->relative_linenr += temp->relative_linenr;
        from->relative_offset += temp->relative_offset;
        temp = temp->parent;
    }
    from->relative_linenr += temp->relative_linenr;
    from->relative_offset += temp->relative_offset;
    to->relative_linenr = from->relative_linenr;
    to->relative_offset = from->relative_offset;

    // alter the children
    if (to->left) {
        to->left->relative_linenr += original_relative_linenr;
        to->left->relative_offset += original_relative_offset;
        to->left->relative_linenr -= to->relative_linenr;
        to->left->relative_offset -= to->relative_offset;
    }
    if (to->right) {
        to->right->relative_linenr += original_relative_linenr;
        to->right->relative_offset += original_relative_offset;
        to->right->relative_linenr -= to->relative_linenr;
        to->right->relative_offset -= to->relative_offset;
    }
}

/* api specified methods */
MetaData *md_new(wchar_t src[static 1]) {
    MetaData *result = malloc(sizeof(MetaData));
    ArrayList *alist = alist_new();
    // put all offsets in the list
    wchar_t *idx = src;
    while (idx != (void *)0x4) {
        alist_push(alist, (ssize_t)(idx - src - 1));
        idx = wcsstr(idx, L"\n") + 1;
    }
    // note that alist->data[0] is -1, this is intentional so we can callculate
    // 1-indexed offsets easily create tree structure recursively
    result->root = node_create(alist->data, 0, alist->used, 0, nullptr, 0);
    result->rows = alist->used;
    alist_free(alist);
    return result;
}

// get byte offset of linenr
size_t md_get_line_start(MetaData *md, size_t linenr) {
    if (linenr > md->rows) {
        cr_log_error("md_get_line_start: invalid line number");
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

void md_insert(MetaData *md, size_t linenr) {
    if (linenr > md->rows) {
        cr_log_error("md_insert: invalid line number");
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
    MetaDataNode *target = node_seek(md, linenr);
    MetaDataNode *node = target;

    // shift relative linenr
    md_shift_linenr(md, linenr, 1, node);

    // shift relative offset
    md_shift_offset(md, linenr + 1, 1, nullptr);

    // insert and rotate
    MetaDataNode *new = malloc(sizeof(MetaDataNode));
    // correct error

    if (target->left) {
        // fix error made in the earliler while loop
        target->left->relative_linenr++;
        target->left->relative_offset += 2;
    }

    // magical number, check the charts and think about them!
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

// delete the given line break
void md_delete_line_break(MetaData *md, size_t linenr) {
    if (linenr > md->rows || linenr == 0) {
        cr_log_error("md_delete_line_break: invalid line number");
        return;
    }
    // update count
    md->rows--;

    MetaDataNode *target = node_seek(md, linenr);

    // update the relative linenr first
    md_shift_linenr(md, linenr + 1, -1, nullptr);

    // update relative offset
    md_shift_offset(md, linenr, -1, target);

    // simple case: one of the branches is null
    if (!target->left || !target->right) {
        remove_single_node(md, target);
        balance_ancestor_of_node(target, node_get_depth(target->left),
                                 node_get_depth(target->right),
                                 target == target->parent->left, md);
        return;
    }
    // if it's not the simple case, we have to create it on our own
    // we find the next smallest element in the subtrees
    MetaDataNode *node = target->right;
    while (node->left) {
        node = node->left;
    }

    // now we swap the target with this element we found
    md_node_mov(target, node);

    // notice now this element is the simple case, before we remove it, we
    // record its parent so we can rebalance it later
    MetaDataNode *unbalanced = node->parent;
    remove_single_node(md, node);
    balance_ancestor_of_node(unbalanced, 0, node_get_depth(unbalanced->right),
                             true, md);
}

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
