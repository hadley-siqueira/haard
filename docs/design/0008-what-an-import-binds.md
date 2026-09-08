# 0008 — An import binds the module's symbols, flat and not transitively

Status: decided, 2026-09-01

## Context

Subject 1.6: `import a.b.c` names a file (record 0001), but it was never decided
what the importer *gets* — the qualified name `a.b.c` to reach through, or the
module's declarations directly in its own scope.

## Decision

**An import binds every symbol the module declares, unqualified, in the
importer's scope.** Functions, classes, structs, enums, unions and global
variables all come in. With record 0005 there is no filtering to do: everything
the module declares is everything the importer sees.

Three rules go with it.

**It is not transitive.** `A` importing `B` sees what `B` declares, and nothing
`B` imported. To use `C` from `A`, `A` imports `C` itself.

**The importer's own declarations win.** A name declared in `A` shadows the same
name coming from any import, with no error and no ambiguity.

**An alias opens a qualified form, and adds to the flat one.** `import b as p1`
and `import c as p2` let `p1::foo` and `p2::foo` name the two `foo`s apart —
while both still come in unqualified as well. The alias is an extra way to reach
a name, never a way to withhold it (record 0009). It must be written to exist:
an import with no `as` gets no qualified form, not even by its last segment,
because a name nobody wrote is a name nobody can grep for — the same argument
that settled the no-self-reference rule.

The grammar already has exactly this and no more: `parse_scope` reads
`identifier '::' identifier`, one alias and one name, which is why
`std::io::println` parses as `std::io` with a leftover
(`tests/parser/cases/let_scope_chain.hd`).

> **Amended by [0012](0012-overloading-and-per-module-tables.md), 2026-09-01.**
> Haard has function overloading, so a name is a list of symbols and not one,
> and "the importer's own declarations win" now applies to **identical
> signatures only**. The importer's `foo(i32)` does *not* hide an imported
> `foo(f32)`: both are candidates. This is explicitly not C++'s name hiding.
> Everything else in this record stands.

> **Amended by [0017](0017-the-prelude.md), 2026-09-02.** There is exactly one
> implicit import: the prelude, one module every compilation loads, whose
> declarations are in scope everywhere without being written. It is searched
> **after** the imports, so anything a file declares or imports beats it. Every
> other module still obeys the rule above.

## Consequences

- **Name lookup is: my own declarations, then the imports.** Two levels, no
  chain to walk, because nothing is transitive. That is what keeps the flat
  namespace from growing without bound once record 0005 (everything is public)
  and record 0006 (a star import is a whole directory) are both in play.
- **A dependency's own dep table is not needed to resolve a name in the
  importer.** It is still needed for two things — knowing what to rebuild, and
  instantiating a generic, whose body was pre-resolved in the declaring module's
  scope (record 0002) and may point at symbols in *that* module's dependencies.
  So the table stays in the blob; it just has no part in ordinary lookup.
- **Adding an import to `B` never changes what a name means in `A`.** Removing
  one can, by taking away a declaration `B` re-exported — except nothing is
  re-exported, so it cannot. Non-transitivity is what makes that true.
- Every exported symbol carrying a **pre-computed name hash** in the blob, which
  the format already planned, is exactly what this lookup wants.

## Settled the same day

Everything this record left open was decided on 2026-09-01, in the two records
next to it: the lookup order, the collision between two imports and the meaning
of `::name` in [0009](0009-name-lookup-and-collisions.md), and the star
import's alias in [0006](0006-star-import-is-a-directory.md).
