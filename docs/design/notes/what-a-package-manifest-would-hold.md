# What a package manifest would hold

Written 2026-09-08, rescued from `exemplo/` so that directory can go.

Record [0010](../0010-a-table-per-root.md) decided that `hdc` reads **one table
of roots** and searches nowhere else, and left the thing above it open:

> **Version resolution.** `hdc` never sees a version — the table holds paths
> only — so this can be designed later without touching the resolver. [...]
> The `haard.pkg` sketch is where it will land, and it is **not decided**.

This is that sketch, and the walkthrough it came with. Nothing here is decided;
it is the shape the example was drawn in, kept because the record points at it.

## The one file a user writes

```
# app/haard.pkg
name = app
version = 0.1

[dependencies]
std = 1.0
zip = 2.0
png = 1.0
```

and one beside every library:

```
# sys/zip/2.0/haard.pkg
name = zip
version = 2.0

[dependencies]
std = 1.0
```

**zip 1.0 does not appear in the app's file.** `png` is what needs it, and
png's own manifest is what declares it. That is the whole of what a manifest
is for here: what *this* thing needs, by name and version, and nothing about
where anything sits on disk.

## What a build system makes of it

```
# generated/roots.tbl -- GENERATED, nobody edits it by hand
root ../sys/std/1.0 = std

root ../sys/zip/1.0 = zip
    std -> ../sys/std/1.0

root ../sys/zip/2.0 = zip
    std -> ../sys/std/1.0

root ../sys/png/1.0 = png
    std -> ../sys/std/1.0
    zip -> ../sys/zip/1.0        <- png sees 1.0

root ../app = app
    std -> ../sys/std/1.0
    zip -> ../sys/zip/2.0        <- the app sees 2.0
    png -> ../sys/png/1.0
```

**No version appears in the table.** `1.0` and `2.0` are path segments and mean
nothing to the compiler, which is what keeps version resolution outside it.

## The scenario, and why it is the interesting one

The app wants zip **2.0**. It also uses png, and png was written against zip
**1.0**. Both write exactly the same text: `import zip.compress`.

| the file writing the import | its root | writes | resolves to |
|---|---|---|---|
| `app/main.hd` | `app` | `import zip.compress` | `sys/zip/2.0/compress.hd` |
| `png/1.0/encode.hd` | `png` | `import zip.compress` | `sys/zip/1.0/compress.hd` |
| `zip/2.0/compress.hd` | `zip` (2.0) | `import zip.utils` | `sys/zip/2.0/utils.hd` |
| `zip/1.0/compress.hd` | `zip` (1.0) | `import zip.utils` | `sys/zip/1.0/utils.hd` |

The first two rows are the point: same text, different files, **and no source
had to be edited** — least of all png's, which is somebody else's code.

The last two are the other point and the easier one to miss: zip 1.0 imports
**itself** and lands in 1.0. With one global table it would land in 2.0,
silently — a library compiling against another version of its own files.

That half is not a sketch any more: it is
`tests/compilation/cases/two_versions_of_one_library`, whose golden holds both
`zip.compress` modules and both `zip.utils` at once, and
`tests/module_finder/` has twenty-one cases on the table's own grammar.

## What is still open above the table

- how versions are **picked** (Go's MVS, npm's tree, Zig's content hash…), and
  the survey of those is in
  [how-other-languages-resolve-imports.md](how-other-languages-resolve-imports.md)
- whether the manifest is `key = value` at all, or something a Haard program
  can read
- `--root name=path` on the command line, the develop-together escape hatch
  every ecosystem eventually grows. It is a convenience over the same
  mechanism and changes nothing in record 0010
