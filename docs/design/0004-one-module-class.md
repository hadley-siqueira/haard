# 0004 — One module class, built and loaded, instead of two

Status: decided, 2026-08-24

## Context

The first sketch had two types: a mutable one holding the vectors while a
module is compiled, and an immutable one holding a loaded blob as spans over a
single buffer, so that reading a dependency needed no deserialization at all.

Against that stood an existing intent: `Context` is meant to be **reused**.
Its state is in `std::vector`s so that compiling the next module is a `clear()`
away, keeping the memory already allocated.

## Decision

One class. A module is always vectors in memory. Reading a blob is a `resize()`
and one `memcpy` per section, straight into those vectors.

## Consequences

- **Reuse and caching turn out to be the same mechanism.** The compilation owns
  a pool of module slots; a slot is free (cleared, memory retained) or taken. A
  module that is checked and already serialized can be evicted back to the pool
  and read again later with one `fread`. There is no separate "reuse pool" and
  "module cache".
- A slot may be recycled once no unchecked module still depends on it. The
  eviction policy itself is 2.3 and still open.
- What is lost is `mmap`'s per-page laziness. Laziness per *section* survives
  and costs nothing: the ast section is not read until a generic is actually
  instantiated.
- The rejected alternative — one class holding either spans or vectors — would
  put a branch in every accessor on the hot path, and templates are out of the
  question for the bootstrap compiler.
