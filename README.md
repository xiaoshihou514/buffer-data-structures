# Benchmarks of different data structures for a text buffer

Read more about the internals of these data structures [here](https://github.com/xiaoshihou514/buffer-data-structures/blob/main/OVERVIEW.md)

## Benchmarks

### Benchmarking aspects:

- creation: is it snappy enough or does it lag when you open a buffer?
- report: get the text in a range, used when rendering buffer
- insert: we do want to mutate the buffer
- search: and traversing it

### Sample files:

    - TODO

## Links

- [gap buffers vs ropes](https://coredumped.dev/2023/08/09/text-showdown-gap-buffers-vs-ropes/)
- [piece tables and piece trees](https://code.visualstudio.com/blogs/2018/03/23/text-buffer-reimplementation)
- [jetbrains fleet buffer internals (ropes)](https://blog.jetbrains.com/fleet/2022/02/fleet-below-deck-part-ii-breaking-down-the-editor/)
