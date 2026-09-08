# 0006 — `import a.b.*` is every module in the directory

Status: decided, 2026-08-24

## Context

Record 0001 made every module path a file path, which left `import std.io.*`
without a meaning: the whole of `std/io.hd`, or the directory `std/io/`?

## Decision

`import std.io.*` imports **every `.hd` file in the directory `std/io`**. It is
shorthand for writing one import per file in that directory.

## Consequences

Two of them touch the cache, and both are easy to get wrong:

1. **A star import depends on a directory listing, which is not a file the
   compiler read.** Adding `std/io/tcp.hd` changes what `import std.io.*` means
   without changing one byte of any source file. So the dependency record for a
   star import must store **the sorted list of module names it expanded to**,
   and that list's hash belongs in the cache key. Without it, a new file is
   silently ignored until something unrelated forces a rebuild.

   > **Narrowed by [0015](0015-the-cache-is-best-effort.md), 2026-09-02.** There
   > is no cache key made of hashes any more: a `.hdm` is valid while its `.hd`
   > has the same size and mtime. The sorted list is still stored, and its hash
   > is not: the `.hdm` is valid only while that stored list **equals** the
   > directory's listing today, compared directly. This is the one change the
   > size-and-mtime rule cannot see on its own, and comparing the list is how it
   > sees it.

2. **The listing must be sorted.** `readdir` order is filesystem dependent, and
   an unsorted expansion would leak that order into name resolution and into
   diagnostic order — which the golden-file suites would see as intermittent
   failures. Sorting once at expansion makes it deterministic.

## Decided on 2026-09-01

Both of the questions this record left open are now answered.

**`*` does not reach subdirectories.** Only the direct children of the
directory, which is the reading that matches "every `.hd` file in the
directory". `import foo.*` over `foo/bar.hd`, `foo/baz.hd` and
`foo/sub/foobar.hd` brings the first two and not the third.

**`import std.io.* as io` gives one alias to the whole expansion.** Everything
the star brought in is reachable as `io::name`, exactly as it is reachable
unqualified — the alias adds and does not withhold (record 0009). It does not
name one module, because there is no one module to name; it names the pool.

The consequence is that the alias cannot disambiguate *inside* the star: if two
modules in the directory both declare `foo`, then `io::foo` is as ambiguous as
the bare `foo` and resolves the same way, to the first in the sorted expansion.
**Separating them is the programmer's job** — a second, explicit `import
std.io.tcp as tcp` next to the star, which is precise because it names one
module. Every alias is written by hand; nothing is qualified automatically.
