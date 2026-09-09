# 0015 — The cache is best effort: size and mtime, or rebuild the whole module

Status: **decided**, 2026-09-02. This record exists to stop a kind of work
before it starts, so it says as much about what the compiler will *not* do as
about what it will.

## Context

Several records had begun to design around cache precision. Record 0005 warned
that everything being public makes `interface_hash` coarse and that this *"has
to be compensated in how it is computed"* — over a canonical form of the
signatures, so that a comment would not rebuild every dependent. Record 0006
asked for the hash of a star import's expansion in the cache key. Agenda 3.4 was
accumulating hashes.

Hadley, 2026-09-02, stopping it: *"if a cache can be had, good. If not,
rebuild to be safe. The goal is to keep the compiler's own code simple."*

The scenario that actually matters is worth writing down, because it is the one
every rule below is measured against: **a programmer downloads dependencies,
they are compiled once, and then they are not touched for weeks while the
programmer edits a handful of their own files.** In that shape the dependencies
sit in the cache by simply never changing. Precision buys nothing there, and
every mechanism bought to gain it is code that has to be written, kept correct,
and rewritten in Haard.

## Decision

**A `.hdm` is valid if its `.hd` has the same size and the same modification
time. Otherwise the `.hdm` is rebuilt from scratch.**

Three things follow from how that sentence is written.

**No hashing of source.** No `source_hash`, no `interface_hash`. Two numbers the
filesystem already has, compared at load.

**From scratch, and never in part.** Nothing in a stale `.hdm` is salvaged — not
a section, not a table, not a declaration that obviously did not change. A module
is valid whole or rebuilt whole. This is the clause that removes the most code:
partial reuse needs a way to say which parts changed, which is the analysis this
record declines to do.

**Recompiling more than strictly necessary is not a defect.** Hadley's list of
things that may cause a needless rebuild, all of them explicitly fine: changing
machine, changing compiler version, and adding a comment. So record 0005's
"compensation" is **retired**: `interface_hash` does not need to be taken over a
canonical form of the signatures, because it does not need to exist.

**Equality, not newer-than.** `make` asks whether the source is *newer* than the
target; this asks whether the timestamp is *the same*. It costs the same and it
also catches a file that moved backwards in time, which happens whenever an
older version is restored over a newer one.

## Decided: a star import compares its expansion

The rule above has exactly one blind spot, and it is closed by a list
comparison rather than by an exception to the rule.

Record 0006 established that adding `std/io/tcp.hd` changes what
`import std.io.*` means with no source file modified, and record 0009 made it
sharper: with first-import-wins, a new file that sorts earlier changes **which
declaration a name resolves to**. The importer's `.hd` is untouched, so size and
mtime would keep a stale `.hdm`.

**So a `.hdm` holding a star import is also valid only while the sorted list of
names it expanded to equals the directory's listing today.** Any difference —
a file added, removed or renamed — rebuilds the module whole, like everything
else here.

It adds no structure: record 0006 already decided the `.hdm` stores that sorted
list. It adds no hash, in keeping with the rest of this record — the lists are
compared directly. And it is bounded, because 0006 also decided that `*` does
not reach subdirectories, so the check is one listing of one directory's direct
children per star import.

This gives record 0006's sorting requirement its third reason to exist. It was
asked for so that `readdir` order could not leak into diagnostics, then record
0009 made it decide *which declaration a program means*, and now it is also what
makes this comparison a comparison rather than a set operation.

## Consequences

**Rebuilding is transitive even though naming is not.** Record 0008 makes an
import non-transitive, which is tempting to read as "if A's source is unchanged,
A's `.hdm` is still good". It is not enough: record 0002 instantiates generics in
the importer, from a body pre-resolved in the *declaring* module's scope, so a
change in B can change what A's generic means without touching a byte of A. The
rule that keeps this safe is the precautionary one and it is still simple:

> a `.hdm` is valid if its own `.hd` is unchanged **and** every module it
> depends on is valid.

Validity propagates; nothing is hashed to decide it. Record 0007 already forces
the compiler to handle cycles in that graph, so this reuses a shape it must have
anyway.

**The blob must carry a version stamp, and it is now load-bearing.** Size and
mtime cannot see a change of compiler. Nothing about a `.hd` changes when the
toolchain does, so a blob written by an older `hdc` would pass the test and be
read with the wrong layout — or, after [0014](0014-the-name-hash.md), with a
name hash that no longer matches. One `u32` in the header, compared at load,
mismatch means rebuild everything. That is agenda 3.6, and this record is what
makes it the only thing standing between a toolchain upgrade and a silently
misread cache.

**Cheap to be wrong in one direction only.** Every rule here fails toward
rebuilding. A needless rebuild costs seconds in the scenario above; a wrongly
reused blob costs a miscompilation that looks like a language bug.

## Rejected

**`interface_hash` over a canonical form of the signatures** (record 0005's
consequence, subject 3.4). It exists to answer "did B's *interface* change, or
only its bodies", so that a comment in B does not rebuild B's importers. It is
the single largest piece of machinery this record removes, and the thing it buys
— skipping a rebuild of the files that import the one file being edited — is
worth less than the code it costs.

**Partial reuse of a stale `.hdm`.** Same argument, larger.

**`make`-style newer-than comparison.** Same cost, sees less.

