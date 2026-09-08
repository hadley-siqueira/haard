# 0033 — The prelude is a list of automatic imports

Status: **decided**, 2026-09-06. It **supersedes the mechanism of record
[0017](0017-the-prelude.md)** and keeps that record's purpose. Hadley proposed
it against a plan that was one step from being built, and it is smaller in
every direction.

| | |
|---|---|
| There is **no prelude module** | **decided** — 0017's mechanism is retired |
| The table carries a list of imports **every module of the program gets** | **decided** |
| They are appended to the **dependency list**, and the Ast is never touched | **decided** |
| The list is resolved **once, against the table**, and not per importing root | **decided** |
| The `prelude` block is a **root with no directory** | **decided** |
| A program with no table, or a table with no `prelude` block, simply has no prelude | **decided** |

## What record 0017 was going to cost

0017 says: one module every compilation loads, whose declarations are in scope
in every file without being imported — *the single exception to record
[0008](0008-what-an-import-binds.md)*. It was one design session from being
built when the cost showed up.

**A prelude module cannot hand on what it imports.** Record 0008 makes an
import non-transitive, and `NameResolver::module_symbol` looks only in a
dependency's **own** module scope, by construction:

```cpp
// only the module scope. Record 0008 binds what a module declares at its
// top level and nothing it imported, so a dependency's own imports are not
// reachable through it
```

So a prelude that wrote `import std.string` would give `String` to nobody. The
prelude would have to **declare** what it offers — `String` moving out of
`std/string.hd` and into the prelude, and `Array`, `List` and `Hash` after it,
in one file every program loads. The way out of *that* was to make the prelude
a directory expanded like record [0006](0006-star-import-is-a-directory.md)'s
star, which is an amendment to 0017 on top of a consequence 0017 did not see.

Hadley, 2026-09-06: *"Ao invés de ter um arquivo de preludio, não dá para
simplesmente inserir automaticamente um import std.string, import std.array etc
em cada módulo compilado?"*

## Decision

**The table lists imports that every module of the program is given.** There is
no prelude module, no implicit scope and no new step in the lookup.

```
prelude
    std -> ../lib/std
    import std.string
    import std.io

root std = ../lib/std

root myapp = myapp
    std -> ../lib/std
```

### Why this is a smaller change than 0017

**It opens no exception to record 0008.** Every name still arrives through an
import; the table writes some of the imports for you. The prelude stops being a
thing in the *language* and becomes a thing in the *build configuration*, which
is where record [0010](0010-a-table-per-root.md) already put every question
about finding a file: the compiler is told, and searches nowhere.

Record 0008's objection — *a name nobody wrote is a name nobody can grep for* —
applies to both designs equally, and this one answers it better. The name **is**
written. It is written in the table, in a block a reader can take in at a
glance, instead of being the implicit content of a directory.

**And `String` does not move.** It stays an ordinary module of the standard
library, reached by an ordinary import, which is what record
[0022](0022-the-standard-library-and-what-is-sugar.md) says it is.

## The three things this record settles that the idea did not

### 1. The Ast is never touched

The proposal was to insert import nodes into each module's tree. It is not
necessary and it is worse:

- **`get_dependencies()` has exactly two consumers**, both in `NameResolver` —
  the flat walk and the alias walk. The dependency list is the whole of what a
  lookup reads, so appending to it is the entire mechanism. `Compilation::
  resolve_imports` already ends in `add_dependency`; this adds a second loop
  after the first.
- **The PrettyPrinter is an oracle.** It walks and works nothing out, and the
  parser suite's verdict is that it prints back what the author wrote.
  Synthetic import nodes would make it print imports nobody wrote, and hiding
  them needs a flag on a node — mechanism, for nothing.
- **A synthetic node has no position.** `Compilation::report` points a
  diagnostic at an import's offset and length. A bad prelude entry is not an
  error in anybody's source: it is an error in the **table**, which the
  `ModuleFinder` already has a discipline for — it does not log, and a
  malformed table is `load` returning false with a message, not a diagnostic.

**Record 0009's order falls out.** Appended after the written imports, the
prelude is last in the list the flat walk reads, which is exactly *the prelude
after the imports*. And `gather_in_module` runs before any of it, so a program
that declares its own `Array` still wins, with no line written to say so.

### 2. The list is resolved once, against the table

`ModuleFinder::target_of` is `visible_root(root, segments[0])`: an import
resolves against the visibility list of **the importing module's root**. If the
entries were resolved per module, `import std.string` would fail in every root
whose block does not write `std ->`, once per module, blaming an import nobody
wrote.

So the list is resolved **when the table is read**, before any module is
loaded, and a bad entry is one table error. This is also what the prelude *is*:
a property of the **program**, not of each root.

### 3. The `prelude` block is a root with no directory

The first idea was a global lookup by root name. It does not work: **block
names are not unique**. Record 0010 supports two versions of one library in one
program, and `tests/module_finder/cases/two_versions_of_one_library` has two
blocks both named `zip`. A global `std.string` would be ambiguous the moment a
program had two `std`s.

So the `prelude` block carries **the same visibility lines a root block
carries**, and its imports resolve through them:

```
prelude
    std -> ../lib/std
    import std.string
```

Which makes it a `Root` with an empty path, and the resolution is `find(prelude
block, "std.string")` — **the existing function, unchanged**. `read_dependency`,
`add_visible`, `visible_root`, `block_with_path` and the second pass all apply
to it as written. The only new syntax is the `import ` line, and it is spelled
the way Haard spells it so there is nothing to learn.

Two guards come with the empty path, and only one of them is real:

- **`root_of_file` must skip it.** `starts_with(file, "")` is true for every
  file, so a prelude block with no guard would claim every file that no real
  root covers.
- **`block_with_path` needs nothing.** A path there always comes from
  `normalize(directory / text)` with the text non-empty, so it can never equal
  the empty one. Nothing is written to defend against a state the design
  excludes.

## Consequences

- **The prelude is optional, and that is not a concession.** 89 of the 142 test
  cases have no table at all, and `hdc file.hd` is a supported mode. A program
  with no prelude has no `Array` and no `String` in scope, said out loud at the
  use, and not a compiler that refuses to run.
- **Three cases are a `u32` comparison**, because a dependency is a module
  index and not a name: a prelude module importing itself (`std/string.hd`
  given `std.string`), a module that also **wrote** the import, and a prelude
  module importing another one. The second is not cosmetic —
  `NameResolver::gather` appends without de-duplicating, so the same candidates
  twice is a **false ambiguity** at a call.
- **`import std.io` stops being necessary**, which was item 4 of `docs/STATE.md`,
  by putting `std.io` in the list. Nothing was built for it.
- **It lands in record [0015](0015-the-cache-is-best-effort.md)'s rebuild graph
  for free**, since these are real dependencies. That was open point 3 of
  record 0017, and it needed a special case there.
- **`String` needs no `std.io`.** It writes `import std.io` today and uses
  nothing from it — checked. So the prelude does not drag IO behind it.
- **What the prelude declares is per program.** Two tables, two preludes, and
  `hdc` alone promises no `Array`. That is the price of the prelude being
  configuration, and it is the same price record 0010 already charges for
  everything else. The generator always writes the same list.

## What was deliberately left out

**The diagnostic does not mention the automatic imports.** It was planned and
dropped while building. `cannot find 'String' in this scope` is what a name the
prelude did not supply still says, and the extra sentence would land on every
not-found error in the language, including the 89 test cases and every
`hdc file.hd` that has no table at all.

There is also nowhere clean to put it: the `Logger` has `error` and `warning`
and no note or help kind, so the text would have to be glued onto the message
itself. Giving it one belongs with the diagnostics work and not here.

## What of record 0017 survives

Its **purpose**: `T[]` finds `Array` without an import, a string literal gets a
type, and `import std.io` stops being written. Its **mechanism** — an implicit
scope and an exception to record 0008 — is retired, and its four open points go
with it:

1. *Where the prelude lives* — the table, and it is a list and not a place.
2. *Its module name, and `prelude::Array`* — there is no prelude module to
   name. Qualification is `import std.string as s`, which record 0008 already
   gives.
3. *Whether it is in the dependency list* — it **is** the dependency list.
4. *What else it declares* — it declares nothing. What it **imports** is a line
   in the table, and the answer can change without a record.

Record [0032](0032-how-a-template-string-is-lowered.md) is unaffected in
substance: the lowering pass still needs to name `String`, and now it finds it
the way any other name is found.
