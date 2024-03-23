# Benchmarks of different data structures for a text buffer

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

We put meta data like line positions in a tree, the tree is a balanced tree with relative offsets to the parent node, reading and writing at arbitrary position O(log n) instead of O(n)

- creation(source):
  complexity: O(n)
  typical divide and conquer, we transform an array into a sorted heap
- get(line_number);
  complexity: O(log n)
  simply traverse the tree
- shift(from, amount);
  complexity: O(log n)
  It's hard to explain, check out the picture here
- insert_line(line_number, at)

### rope

TODO

### piece tree

TODO

## Benchmarks

### Benchmarking aspects:

    - report: get the text in a range, used when rendering buffer
    - insert: used ... when you insert stuff
    - search: ...

### Sample files:

    - TODO

## Links

    - [gap buffers vs ropes](https://coredumped.dev/2023/08/09/text-showdown-gap-buffers-vs-ropes/)
    - [piece tables and piece trees](https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation)
    - [jetbrains fleet buffer internals (ropes)](https://blog.jetbrains.com/fleet/2022/02/fleet-below-deck-part-ii-breaking-down-the-editor/)
