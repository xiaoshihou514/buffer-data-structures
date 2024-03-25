## Introduction

### gap buffer

Idea: a buffer of characters with a "gap" so we don't have to resize it as much

- creation(source):

  simply allocate the space for the gap and the source and do a memcpy, initially the gap is at the start

- read(start_row, start_col, end_row, end_col):

  we read the metadata to get the offset, see if we have to skip a gap, and just memcpy twice

- insert(pos, string):

  we move the gap to `pos` and fill it in, reallocating as needed

- search(needle):

  trivially traverse the buffer, skipping the gap

Implementation: buffer.h, buffer.c

#### accompanying metadata

We put meta data like line positions in a tree, the tree is an avl tree with relative offsets and line numbers to the parent node, reading and writing at arbitrary position is O(log n) instead of O(n)

- creation(source):

  complexity: O(n)

  typical divide and conquer, we transform an array into a sorted heap

- get(line_number);

  complexity: O(log n)

  simply traverse the tree

- shift(from, amount);

  complexity: O(log n)

  It's hard to explain, check out the picture [here](https://github.com/xiaoshihou514/buffer-data-structures/blob/main/resources/metadata_tree_shifting.png)

- insert_line(at)
  complexity: O((log n)^2)
  avl tree insertion

- delete_line(at)

### rope

TODO

### piece tree

TODO
