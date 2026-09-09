# 0010 — A table per root: the compiler is told, and it is told once per root

Status: **decided**, 2026-09-01. Confirmed by Hadley after reading the worked
example, which lived in `exemplo/` until 2026-09-08 and is now
[notes/what-a-package-manifest-would-hold.md](notes/what-a-package-manifest-would-hold.md)
plus `tests/compilation/cases/two_versions_of_one_library`.

This record was first written on the same day as *partly decided*, against the
ledger's usual rule, because two halves were settled while a third was not. The
third is now settled and the record is whole. Nothing that was decided then has
changed.

## Context

Agenda 1.9 asked where an import's root comes from. Record 0001 answered "an
environment variable in the style of `CLASSPATH`" and **this record supersedes
that sentence**. `proposta_import.txt` replaced it with a configuration table,
and the 2026-09-01 conversation replaced the question itself: the choice is not
between one root and a list of roots, it is between two shapes.

- **A** — one global `name -> path` map for the whole compilation. Java, Python,
  C, OCaml, Go.
- **B** — one such map **per root**, so what `import zip.foo` names depends on
  which root the importing file sits in. Rust, Zig, C#, Haskell, npm (carried by
  directory nesting rather than a table), Yarn PnP.

A **root** is the directory an import's first segment names: a whole
self-contained library. `zip.compress.utils` is `<zip's root>/compress/utils.hd`.

## Decided: the compiler is told, and never searches

`hdc` is handed a finished mapping and does no searching of its own. No ordered
list, no first-match-wins, no speculative `stat`. An import that matches no
entry is one diagnostic naming the entry that is missing, never a different file
found by accident.

This is Family 1 in `notes/how-other-languages-resolve-imports.md` — Rust's
`--extern name=path`, Zig's `-M name=path`, C#'s `/reference`, Haskell's
`-package-id`, Yarn PnP's table.

## Decided: A is rejected

Hadley, 2026-09-01: *"Definitivamente não usaremos A pois não quero que seja
como é em C++."*

**The reason it is rejected is real, and the reason given is not quite the
mechanism.** This distinction has to survive into the future or someone will
draw the wrong conclusion from this record:

- What makes a C++ repository fail to build is **ambient search** (`-I` order
  decides which header you got), dependencies that must be **pre-installed** by
  hand, and the mapping living **outside the repository** in a makefile. That is
  the axis above, and it is fixed by the compiler being told — in A *or* in B.
- What A costs is different and narrower: **one global name means one version,
  ever.** Java, Python and C all sit there, and every workaround in those
  ecosystems is a way to manufacture a second name — shading, virtualenvs,
  symbol versioning.

So A-with-an-explicit-table would already have cured the C++ symptom. It is
rejected for the second reason, and the practical form of that reason is the one
that matters to this project: **with A a resolver can legitimately fail.**
`pip` and Maven do report "no set of versions satisfies everyone" and hand the
problem back to a human. `npm` and Cargo never do, because they are free to
nest or duplicate. Hadley named npm and Cargo as the experience he wants; not
failing is part of that experience, and it is bought by B.

The one version of A worth remembering as rejected-with-honour is **Go's**: one
version per major, with the major written into the name (`x/y/v2`). It is the
simplest design surveyed and the one users praise most, and it stays reachable
as a *policy* — see below.

## Decided: B, a table per root

Demonstrated in `tests/compilation/cases/two_versions_of_one_library`, whose
golden holds both versions of one library in one compilation:

```
root ../sys/zip/1.0 = zip
    std -> ../sys/std/1.0

root ../sys/zip/2.0 = zip
    std -> ../sys/std/1.0

root ../sys/png/1.0 = png
    std -> ../sys/std/1.0
    zip -> ../sys/zip/1.0

root ../app = app
    std -> ../sys/std/1.0
    zip -> ../sys/zip/2.0
    png -> ../sys/png/1.0
```

`app/main.hd` and `png/encode.hd` write the identical `import zip.compress` and
reach different files, with no source edited. `= zip` says "inside this
directory the name `zip` is this directory itself", which turns the self-name
rule into a line of the block instead of a special case in the resolver — and
without it, zip 1.0's own `import zip.utils` would land in zip 2.0 silently.

**The argument that decided it: B is a mechanism and A is a policy.** C# and
Haskell both hand their compilers a per-importer table, and both ecosystems'
build systems unify to a single version anyway. A build system can impose A on
top of B at any time — including Go's answer, by writing the same root for every
consumer. The reverse cannot be done without changing what every import means,
which is the retrofit Node needed Yarn PnP for and Java needed JPMS for, and
neither replaced the old system.

## Decided: the table's format

Plain text, not JSON. It is more readable, and a hand-written JSON parser is
real cost under the no-external-dependencies rule.

A file is a sequence of blocks. A block is a header line and zero or more
indented lines:

```
root <path> = <name>
    <name> -> <path>
```

- A line starting at column zero with `root` opens a block. A line starting with
  any whitespace is a dependency of the block above it. Blank lines are ignored,
  and so is a line whose first non-blank character is `#` — a whole line only,
  never a trailing comment, since a path may hold a `#`.
- Comments were first ruled out on the grounds that the file is generated and
  nobody reads it. The example's own table refuted that before the code was
  written: it opens with a header saying it is generated and must not be edited
  by hand, which is exactly what a reader who stumbles into it needs, and is
  what every other generated file in every other ecosystem carries.
- `<name>` is one identifier — an import's **first segment**, never a dotted
  path. `<path>` is a directory.
- The header's `= <name>` is the root's **own** name, the one its internal
  imports use. It is the same kind of entry as the indented ones, written on the
  header line because it is always present and always points at the block's own
  path.
- Two blocks may carry the same `<name>` — that is the entire point of B. Two
  blocks may not carry the same `<path>`: a block *is* a root, and a root is its
  path.
- **No version appears anywhere.** `1.0` and `2.0` in the example are path
  segments and mean nothing to `hdc`.

Resolution, then, is total and local: given the importing file's root, the
import's first segment is looked up in that root's block, and the remaining
segments become directories with `.hd` on the last. One lookup, one file open.

## Decided: the entry point

`hdc --roots generated/roots.tbl app/main.hd`

`hdc` keeps taking a **file path**. The file's own root comes from a
**longest-prefix** match against the blocks' paths, so `app/main.hd` falls under
`../app` and is the module `app.main`. A file under no root is an error whose
message names the block that is missing.

Longest-prefix is needed rather than any-match because roots nest in practice —
`sys/zip/1.0` sits under `sys`, and a `sys` root must not capture zip's files.

## Decided: relative paths resolve against the table file

A relative `<path>` in the table is relative to **the directory holding the
table**, never to the working directory. Otherwise the same command run from two
directories resolves to two different files and moves the cache key, which
agenda 3.4 depends on being stable.

## Deferred, on purpose

- **Version resolution.** `hdc` never sees a version — the table holds paths
  only — so this can be designed later without touching the resolver. Zig's
  content-hash-as-identity and Go's MVS both stay reachable. The `haard.pkg`
  sketch is where it will land, it is **not decided**, and it is written down in
  [notes/what-a-package-manifest-would-hold.md](notes/what-a-package-manifest-would-hold.md)
  — rescued from `exemplo/` on 2026-09-08, when that directory stopped holding
  anything the suites do not.
- **`--root name=path` on the command line**, the develop-together escape hatch
  that the survey found every ecosystem eventually needs. It is a convenience
  over the same mechanism and changes nothing in this record; it can be added
  when something wants it.

## The costs of B, carried forward

1. **It needs a generator to be good.** A hand-maintained table that grows with
   the dependency graph *is* the `CMakeLists.txt` this design exists to escape.
   The mitigation is that the generator is simple while there is no solver.
2. **Duplicate global state.** Haard has module-level `let` and `const`, so two
   versions of a library are two copies of its globals — Node's duplicate
   instance problem, where `instanceof` across two copies silently fails.
   Nothing warns unless something is built to warn.
3. **Two types under one name.** Two roots of one library declare two
   `zip.archive.Archive`s. Passing one where the other is expected must be an
   error whose diagnostic **names the root of each**, or it reads as Cargo's
   `expected Archive, found Archive`.
4. Duplication costs binary size and compile time; the table is repetitive
   (generated, never read by a user); deduplication is not automatic but falls
   out for free from a module registry keyed by resolved path (agenda 2.3), as
   long as the generator writes identical paths for what it means to share.

## What this supersedes

Record 0001's closing sentence, "The root directory comes from an environment
variable, in the style of `CLASSPATH`", and its first "still open" bullet,
which asked whether the root is a single directory or a list like `PATH` — it is
neither. Nothing else in 0001 changes: a module path is still a file path, and
the mapping is still a plain separator replacement.
