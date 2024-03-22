# Benchmarks of different data structures for a text buffer

## Introduction

    - gap buffer
        - idea: a buffer of characters with a "gap" so we don't have to resize it as much
        - we put meta data like line positions in a tree, the tree is a balanced tree with relative offsets to the parent node, reading and writing at arbitrary position O(log n) instead of O(n)
    - rope
        - TODO
    - piece tree
        - TODO

## Benchmarks

### Benchmarking aspects:

    - report: get the text in a range, used when rendering buffer
    - insert: used ... when you insert stuff
    - search: ...

### Theoretical complexities

|           | Gap Buffer | Rope | Piece Tree |
| --------- | ---------- | ---- | ---------- |
| report    | O(log n)   | TODO | TODO       |
| insertion | O(n)       | TODO | TODO       |

### Sample files:

    - TODO

## Links

    - [gap buffers vs ropes](https://coredumped.dev/2023/08/09/text-showdown-gap-buffers-vs-ropes/)
    - [piece tables and piece trees](https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation)
    - [jetbrains fleet buffer internals (ropes)](https://blog.jetbrains.com/fleet/2022/02/fleet-below-deck-part-ii-breaking-down-the-editor/)
