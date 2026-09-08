# 0013 — The symbol table: a scope of names, a name of candidates, and no merge at the import

Status: **decided**, 2026-09-02. Four structures and one lookup rule. What is
still open is named at the end, and none of it blocks writing the phase.

| | |
|---|---|
| A scope is a list of **names**, and a name is a list of **candidates** | **decided** |
| The name is an **interned index**, and the interned entry carries its **hash** | **decided** |
| An import **does not merge**: the lookup searches the dependency's own table | **decided** |
| `kind` belongs to the **candidate**, not to the name | **decided** |
| A scope points back at the declaration that **opened** it | **decided** |
| Which hash function, pinned so a blob outlives the build that wrote it | **open** |
| Two declarations of one name with different kinds: representable, but not yet ruled on | **open** |

## Context

Agenda 2.5 asked how the symbol table is represented so that it survives
serialization. Record 0012 had already settled two of its axes — one flat table
per module, and *a name maps to a list of symbols, not to one*, because Haard
has overloading — and left the representation open.

A first proposal for the rest was written over two sessions and **rejected in
full** on 2026-09-02. This record is the shape Hadley designed in its place. It
is not a variation on the rejected one: it reaches the same two conclusions that
survived (a symbol is addressed by index, and the name is not duplicated across
every declaration) by a different route, and it is built out of the idiom the
compiler already has instead of introducing a second one.

## Decision

```
struct Symbol {
    u32 name;               // index into the module's interned strings
    u32 candidates;         // head of the candidate list
    u32 sibling_or_next;    // next name in the same scope
}

struct Candidate {
    u8  kind;               // what kind of declaration this is
    u32 ast_node;           // the declaration, in this module's Ast
    u32 next_candidate;     // next declaration of the same name
}

struct Scope {
    u32 parent;             // enclosing scope, 0 at the module scope
    u32 symbols;            // head of the name list
    u32 owner;              // the ast node that opened this scope, 0 at the module
}
```

**A `Symbol` is not a declaration. It is a name inside one scope.** That is the
unit record 0012 asked for when it made a name map to a list: the list is the
candidate chain, and there is exactly one `Symbol` per distinct name per scope.

## Decided: it is the Ast's idiom, applied twice

`Scope.symbols` is a `children` head and `Symbol.sibling_or_next` is a `sibling`
chain; `Symbol.candidates` and `Candidate.next_candidate` are the same pair one
level down. This is `AstNode` exactly:

```
AstNode { u8 kind; u32 token; u32 children; u32 sibling; }
```

So the compiler keeps **one idiom** — a linked list threaded through a flat
vector, everything by index, nothing owning anything — already proven by the Ast
and walked by the `PrettyPrinter`. The alternative that was rejected introduced
a sorted vector searched by binary search, which existed nowhere else and needed
an explicit `seal()` before it could be read.

**The candidate list is a list and not a span on purpose.** Candidates of one
name do not arrive together: they are interleaved with every other declaration
in source order. A span would need a sorting pass; the list keeps the collector
an append and keeps the source order that record 0009 depends on.

## Decided: the name is interned, and the entry carries its hash

`Symbol.name` indexes a table of interned strings **per module**, as record 0012
requires of every table. Each entry carries the string and its hash. The layout
that follows is a char pool plus

```
struct InternedString { u32 offset; u32 length; u32 hash; }
```

both of which are blob sections that load by `memcpy`, keeping record 0004's
promise.

Two comparisons, and they cost differently on purpose:

- **Inside a module**, interning guarantees one entry per distinct string, so a
  name comparison is `u32 ==` and never touches a byte.
- **Across a module boundary**, the hash is the probe and a byte compare
  confirms it. The confirm is not optional: a `u32` over a few thousand names
  collides often enough to matter.

The hash is stored, not recomputed on load. Record 0008 predicted exactly this —
*"every exported symbol carrying a pre-computed name hash in the blob, which the
format already planned, is exactly what this lookup wants"* — a line written
before there was a table to put it in.

## Decided: an import does not merge, it searches

**A module never copies a dependency's symbols into its own table.** Resolving a
name is: the scope chain of the importer, up through `Scope.parent`; then, at
the module scope, the module scope of each imported module, in source order.

This and the stored hash are one decision, not two. Without merging, the
importer's interned id means nothing in the dependency's table, so the
translation the merge used to do once at the import has to happen at every
lookup — which is the string comparison interning was there to remove. The hash
is what makes the un-merged lookup a probe instead.

What it buys:

- **The stored table has no cross-module reference at all.** Nothing to remap on
  load, so a `.hdm` read stays `resize()` plus one `memcpy` per section. This is
  record 0004's promise in its strongest available form.
- **Agenda 3.2's `(dependency index, symbol index)` pair becomes transient.** It
  is the *result* of a lookup — a vector the resolver builds and discards — and
  never a field of a stored structure. 3.2 still holds; it just does not
  describe anything written to disk in this table.
- **An alias needs no structure.** Record 0008 gives `import b as p1` both a flat
  route and a `p1::` route. Under no-merge, `p1::foo` is the same search as the
  flat one, restricted to a single import. Merging would have needed duplicate
  candidate nodes for the second route, because a candidate can only be in one
  chain.

What it costs, and both prices are known:

- **Record 0009's ordering moves out of the structure and into the walk.** With a
  merge, "the first import in source order wins" fell out of insertion order in a
  single chain. Now it comes from the order the resolver visits the imports, so
  **`Compilation` must keep each import's module index in source order.**
  `Compilation::resolve_imports` computes that index today and throws it away;
  this is the decision that makes it have to be kept.
- **A star import probes every dependency.** A directory of fifty files (record
  0006) is fifty hash probes for one unresolved name. The hash keeps each probe
  cheap, and if it ever hurts the fix does not touch the format: memoize the
  resolution in the importer's own scope after the first hit — a lazy merge in
  memory that the blob never sees.

## Decided: `kind` belongs to the candidate

It is derivable from `ast_node`, and it is stored anyway, for two reasons that
are not about speed.

**A candidate's `ast_node` may belong to another module's Ast.** Under no-merge
a lookup returns candidates from a dependency, and a filter over them must not
have to reach into that module's node vector to ask what kind each one is.

**And it makes a conflict representable.** With `kind` on the `Symbol`, a module
declaring both `def foo` and `let foo` could not be written down — and what
cannot be represented cannot be reported. Now the two are candidates of
different kinds under one name, and the diagnostic has both to point at.

Record 0012 amended record 0009 so that the scope steps *contribute to one
candidate set* for functions while still *stopping at the first hit* for
variables. That rule reads the kind of the first candidate of the name it found.

## Decided: a scope points back at what opened it

`Scope.owner` is the ast node of the declaration that opened the scope, and 0 at
the module scope.

It cannot be derived. Record 0009's lookup has an *enclosing class* step, which
record 0012 turned into a filter, and any diagnostic about a method has to be
able to name its class — but `AstNode` carries `children` and `sibling` and no
parent, so from a method there is no way back up. Four bytes on an object that
exists in the dozens closes it.

## Consequences

- **Locals and parameters are in the table.** The rejected proposal kept them
  out; with `Scope.parent` chaining blocks there is no reason to, and the chain
  exists precisely so they can be.
- **`AstQuery::text_of` leaves the hot path.** It allocates a `std::string` per
  call — one of the findings in `notes/what-a-module-costs.md` — and interning
  means a lookup never calls it.
- **Agenda 2.7 is half answered.** How scopes are *recorded* is this record.
  How resolution *runs* is still 2.7's, and its second half stays downstream of
  2.6 and 1.13.
- **Every structure is a multiple of four with no padding waste.** `Symbol` 12,
  `Candidate` 12 with three bytes spare, `Scope` 12, `InternedString` 12. Better
  than `AstNode`, which loses 19% to alignment.

## Rejected

**The first proposal for 2.5**, in full, on 2026-09-02: two flat sections per
module, `Symbol { kind; declaration; owner; }` in declaration order, plus a
`NameEntry` index sorted by `(owner, name text)` so a lookup was a binary search
returning a contiguous run, with no `Scope` type at all and an explicit
`seal()`. It is not re-proposed and this record is not a revision of it.

**`kind` on the `Symbol`**, which the second iteration of this design had.
Superseded above.

**A `u32 module` field on `Candidate`**, considered while the import scheme was
still open. Unnecessary once the import stopped merging: a module's candidates
are only ever its own.

## Open

- ~~Which hash function.~~ **Decided the same day** by
  [0014](0014-the-name-hash.md): FNV-1a, 32-bit, written in the compiler's own
  source rather than taken from the standard library.
- **What `def foo` next to `let foo` actually is.** The table can now hold it and
  report it; whether it is an error, and at the declaration or at the use, is not
  decided.
- **1.13**, the logic of implicit conversion, still blocks overload resolution.
  **2.6**, the type table, is where a signature lives, and this record stores
  none: a candidate is a declaration, and what distinguishes two overloads
  (record 0012: arity, parameter types, generic parameters) is read from the
  type table when resolution ranks them.
