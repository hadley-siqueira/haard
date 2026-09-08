# 0009 — Name lookup: the order, the collision, and `::`

Status: decided, 2026-09-01

## Context

Record 0008 made an import bind its module's symbols flat in the importer's
scope and settled own-versus-imported: the importer's own declaration wins. It
left open which of two *imported* `foo`s a bare `foo` names (subject 1.7), and
what the parser's alias-less `::foo` means (subject 1.11).

## Decision

**Lookup order, innermost first:** the local scope, then the enclosing class,
then the module's own declarations, then the imports.

**Between two imports, the first one in source order wins.** Imports are
ordinary top-level declarations — `parse_declaration` accepts `import` anywhere
a `def` or a `class` may go — so "first" means first in the file, not first in
some sorted list.

**The collision is a warning, and it is reported at the use.** Not at the
import. It names both modules and points at the ambiguous name.

**`::name` is the module scope.** It skips the local scope and the enclosing
class and names what the module declares at its top level. The case it exists
for:

```
def foo():
    ...

class Thing:
    def foo(self):
        ::foo()     # the module's foo, not this method
```

**An alias is additive, not restrictive** (this completes 0008): `import b as
p1` puts B's names in the flat scope *and* under `p1::`. The alias is an extra
way to reach them, never a way to withhold them.

> **Amended by [0012](0012-overloading-and-per-module-tables.md), 2026-09-01.**
> With overloading the four scope steps below stop being filters that stop at
> the first hit and become sources that **contribute to one candidate set**.
> "The first import wins", with its warning at the use, applies to **identical
> signatures only**; two imports bringing a `foo` of different signatures are
> two overloads and both are callable. A call that matches two candidates
> equally well is an ambiguous *call*, reported by overload resolution, not an
> ambiguous *name*. The lookup order itself and `::name` are unaffected.

## Consequences

- **Aliasing does not silence the warning.** Since the alias adds and does not
  remove, `import b as p1` and `import c as p2` leave a bare `foo` still
  ambiguous, still resolving to B's, still warning at every use. The warning
  goes away when the programmer writes `p1::foo`, not when they write the alias.
- **Warning at the use costs nothing for a name nobody uses.** With everything
  public (record 0005) and a star import pulling a whole directory (record
  0006), most collisions are between helpers that the importer never names.
  Reporting at the import would make the common case the loud one.
- **A star import's "first" is the sorted directory listing.** Record 0006
  already required sorting the expansion so that `readdir` order could not leak
  into diagnostics; with first-wins it now also decides *which declaration a
  program means*, which makes the sorting load-bearing rather than merely tidy.
- **The hazard that comes with it, and it is worth knowing:** inside a star
  import, adding a file that sorts earlier and declares an existing name changes
  which declaration a bare use resolves to, with no source byte of the importer
  changed. The warning at the use is what makes it visible; the sorted
  expansion in the cache key (subject 3.4) is what makes it rebuild.
- **`::name` needs no grammar work.** `Parser::parse_scope` already reads a
  leading `::` with no alias and writes it as a scope node with a 0 alias.

## The same module imported twice is not a collision

`import std.io.*` next to `import std.io.tcp as tcp` brings `tcp`'s
declarations in by two routes. There is no conflict and no duplicate-import
error: it is one declaration arriving twice, so first-wins chooses between
equals and the alias only adds a way to name it. This is the intended shape —
it is how a programmer separates one module out of a star import (record 0006).

## Rejected

Reporting the collision at the import. It is cheaper to compute and it is
deterministic without any use-site analysis, and it was rejected because the
noise falls on the innocent: a directory of helpers sharing an obvious name
would warn on every file that star-imports it, whether or not the name is ever
written.
