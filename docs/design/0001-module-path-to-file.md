# 0001 — A module path maps straight to a file

Status: decided, 2026-08-24

## Context

`import foo.bar.baz` had two possible readings: `baz` is a module, or `baz` is
a symbol inside the module `foo.bar`. The parser accepts both shapes already
(`tests/parser/cases/import_simple.hd` is `import std.math.sqrt`), so the
ambiguity had to be settled before anything can resolve an import.

## Decision

A module path is always a file path. `foo.bar.baz` is `foo/bar/baz.hd`, the
separator being `/` or `\` depending on the platform — a plain replacement,
nothing else. The root directory comes from an environment variable, in the
style of `CLASSPATH`.

`import std.math.sqrt` therefore names the file `std/math/sqrt.hd`. It never
names a symbol inside `std/math.hd`.

> **Amended 2026-09-01 by [0010](0010-a-table-per-root.md).**
> The sentence above about a `CLASSPATH`-style environment variable is
> superseded: the compiler is *told* the mapping and searches nothing. The rest
> of this record — a module path is a file path, by plain separator replacement
> — stands unchanged.

## Consequences

- The resolver is a string replacement and a file open. There is no fallback
  search, and no case where a failed file open means "try it as a symbol".
- A module is a file, so a module's name is its path. Nothing else needs to
  carry it.

## Still open

- ~~Whether the root is a single directory or a list like `PATH`, and whether
  the current file's own directory takes part in the search~~ — answered by
  [0010](0010-a-table-per-root.md): it is neither, and there is no search.
- What `import a.b.*` means once every path is a file (1.2).
