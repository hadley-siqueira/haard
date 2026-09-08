# 0017 — The prelude: one module every compilation loads

Status: **superseded**, 2026-09-06, by
[0033](0033-the-prelude-is-a-list-of-automatic-imports.md). Its **purpose**
stands — `T[]` finds `Array` without an import, a string literal gets a type,
`import std.io` stops being written — and its **mechanism** is retired, along
with the four open points at the end, which 0033 answers by not having them.

What it could not have done as written: an import is not transitive (record
[0008](0008-what-an-import-binds.md)), and `NameResolver::module_symbol` looks
only in a dependency's own module scope, so a prelude module that wrote
`import std.string` would have given `String` to nobody. The standard library
would have had to move inside the prelude. 0033 has no prelude module at all:
the table carries a **list of imports** every module of the program is given.

The rest of this file is kept as it was written, because 0033's argument is
against it and reads better next to it.

Status when written: **decided in principle**, 2026-09-02. The mechanism is
settled; four things about it are not, and they are named at the end.

## Context

Record 0016 made `T[]` written form for `Array<T>`, which put a question to
record 0008: that record makes an import bind names and says **nothing is
implicit, not even a module's last segment**, on the argument that a name nobody
wrote is a name nobody can grep for. But `i32[]` writes no import and has to
find `Array` anyway.

Hadley, 2026-09-02: *"Vamos com prelúdio: Array vem de um módulo sempre
carregado."*

## Decision

**One module is loaded by every compilation, and its declarations are in scope
in every file without being imported.**

It is the single exception to record 0008's rule, and it is the smallest one
that makes the sugar work: `Array` is reachable because the prelude declares it,
the way `i32` is reachable because the grammar has it.

## Consequences

- **Record 0008 is amended, not revoked.** Its rule — an import binds what it
  binds and nothing is implicit — holds for every module but this one. There is
  exactly one implicit import in the language and it is named in this record,
  which is what keeps "a name nobody wrote" from becoming a category.
- **The lookup gains a last step.** Record 0009's order — local, enclosing
  class, module, imports — gets **the prelude after the imports**, so anything a
  file declares or imports beats it. A program that declares its own `Array`
  uses its own, silently, which is the same rule record 0008 already gives an
  importer over its imports.
- **It is a module like any other**, with a symbol table, a type table and a
  blob. Nothing in records 0013 and 0016 changes: the prelude is one more entry
  in a module's dependency list, and its types translate at the import like
  anybody's.
- **`Array` stops being magic and starts being a class.** The compiler knows the
  *name* to desugar `T[]` into, and nothing else about it. Its methods, its
  layout and its generic parameter are ordinary Haard.

## Open, and each is small

1. **Where the prelude lives.** Record 0010 has the compiler told about roots
   and never searching, so the table is the natural place — one entry the
   generator always writes. It is not decided that it goes there.
2. **What its module name is**, and whether a program may name it explicitly to
   qualify against it (`prelude::Array`) the way an alias works.
3. **Whether it appears in the dependency list**, and so in the rebuild graph of
   record 0015. It should: it can change, and everything depends on it.
4. **What else it declares.** `Array` is the one thing forced by record 0016;
   whether a string type, a hash and the rest belong there is a language
   question that has not been asked yet.

   **Amended 2026-09-06, record [0032](0032-how-a-template-string-is-lowered.md):
   it also declares `String`.** A template string lowers to a local `String` and
   a run of `append` calls, in whatever module it was written in — a type the
   source never wrote, which is the same problem `T[]` → `Array<T>` posed and
   this record exists for. Hadley chose the prelude over requiring an import,
   so **the prelude now blocks template strings** and comes before them in the
   order of work. Two of the four names of record 0022 are forced now, and the
   list is no longer open in the way it was.
