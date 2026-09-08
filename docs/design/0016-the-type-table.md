# 0016 — The type table: interned structure, translated at the import

Status: **decided**, 2026-09-02. The representation of agenda 2.6. What it
deliberately is not is at the end, and it is most of what a type system does.

| | |
|---|---|
| A type is an **interned structure**, and identity is a `u32` | **decided** |
| One flat entry plus a **shared argument pool**, not a sibling chain | **decided** |
| The builtins hold **fixed indices** in every module | **decided** |
| A type crossing a module boundary is **translated at the import**, not probed | **decided** |
| A named type carries the **pair** `(module, candidate)` — 3.2, stored at last | **decided** |
| The array length is part of a type's identity | **decided** |
| Whose value a non-literal length has | **open** — needs constant evaluation |
| `T&&` is an rvalue reference, needing its own kind | **decided** 2026-09-02 |
| `T[]` is sugar for `Array<T>` | **decided** 2026-09-02 |
| Where `Array` comes from — a prelude, or an import | **open**, and it blocks that sugar |

## Context

Agenda 2.6 asked how the type table is represented. Record 0012 had settled one
axis — it is **per module** — and left the rest.

Today a type is a tree in the `Ast`: `Node<i32>*` is four nodes. Asking whether
two types are the same means walking two trees, and overload resolution asks
that question in its innermost loop. Four things already decided need the
answer to be cheaper than a walk:

- **Record 0012** distinguishes two overloads by arity, parameter types and
  generic parameters. With a type as a `u32`, a signature is a list of `u32`.
- **Record 0004** promises a load is `memcpy`. A tree of `AstNode` cannot cross
  a module boundary; its indices belong to another module's `Ast`.
- **Agenda 1.13**, implicit conversion, is a question about two types and needs
  something to ask it about.
- The `UseResolver` refuses to resolve the right side of an `AST_DOT` precisely
  because there is no type for the left side to have a field of.

## Decision

A type is interned the way a name is: written once into a flat table, and from
then on it *is* its index.

```
struct Type {
    u8  kind;               // builtin, named, generic, pointer, reference,
                            // array, list, hash, tuple, function
    u32 subject;            // the builtin's code, the CANDIDATE of a named
                            // or generic type, or an array's length
    u32 module;             // for a named type, which module the symbol is in
    u32 first_argument;     // index into the argument pool
    u16 argument_count;
}
```

16 bytes, and a `std::vector<u32>` argument pool beside it. Two sections, both
loaded by `memcpy`.

**The arguments are a pool and not a sibling chain, and that is a deliberate
break from the `Ast` idiom that `Symbol`, `Candidate` and `Scope` all keep.**
Interning is the reason. A node of a linked list can only be in one chain, so
`(i32, f32)` built twice produces two chains at two indices, the two parent
entries differ, and the deduplication fails exactly where it is the whole point.
A contiguous run of argument indices is comparable as a unit; a chain is not.

**The deduplication is record 0014's hash**, FNV-1a over the entry's fields and
its argument run, with the bytes confirming a hit — the same mechanism the
`StringTable` uses, over different data. There is one hash function in the
compiler and now two tables using it.

**A function type carries its return as the last argument.** Record 0012 says
the return type does not distinguish two overloads, so resolution reads the
arguments short of the last one and ignores it. One table and no separate
signature table.

## Decided: the builtins are the same index everywhere

The thirteen — `u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 bool void char` — are
seeded by the constructor at fixed indices in every module. So the most common
comparison in the language crosses a module boundary with no translation at all.

## Decided: a type is translated at the import, not probed

Record 0013 decided that a **name** crossing a boundary is not merged: the
lookup probes the dependency's own table by hash. **A type is the opposite: the
importer interns the dependency's types into its own table** and keeps the map
from the dependency's index to its own.

The two are not inconsistent, they are priced differently:

- comparing two names across a boundary is a hash probe and one `memcmp` —
  cheap enough to pay at every lookup;
- comparing two types across a boundary is **structural and recursive**.
  `Node<Pair<i32, f32>>*` against the same in another module descends the whole
  tree, and at every level the index on one side means nothing on the other.

Paying the walk once at the import and `==` forever after is the trade. It does
not weaken record 0004: the blob of A holds A's table, whole and closed over
itself, and the map is built in memory the way the name probe is done in memory.
The price is that A's table grows with what A uses of B, and only with that if
the translation is lazy.

Record 0012 had already sketched this — *"comparing a name across a module
boundary needs a translation at the import"* — and record 0013 found a cheaper
route for names. For types there is none.

## Decided: this is where agenda 3.2's pair is finally stored

Record 0013 made the pair `(module, symbol)` **transient**: the result of a
lookup, never a field. The type table cannot do that. A type is structural, and
a named type must say *which* declaration it names — so `Type::module` and
`Type::subject` are that pair, written down.

**The pair names a candidate and not a symbol**, because a symbol is a *name*
and may hold several declarations: a module may write `class Foo` next to a
`def Foo`, and a type means one of them. A candidate is a declaration, which is
what a type has to point at.

**In the blob it has to be a dependency index**, not a compilation-wide module
index, or the file is not self-contained. That is agenda 3.2's job, and it
brings a question with it: record 0009 allows one module to arrive by two routes
(a star import and an explicit one), so two dependency indices may name one
module and the same foreign type could intern twice. Canonicalising to the first
such index is the obvious answer and it belongs to 3.2, not here. **Until the
blob exists this field holds the compilation's own module index**, which is the
smallest thing that runs and is written here so the change is not a surprise.

## Decided: an array's length is part of its identity

`i32[10]` and `i32[5]` are different types, and two `i32[10]` written in two
places must be one index — so the length is stored **as a value** in `subject`,
not as the expression node that wrote it. Two different nodes holding `10` would
otherwise defeat the interning.

`T[]`, with no length, is its own type and not `T[0]`.

**Open:** a length that is not an integer literal — `i32[N]` for a `const N` —
needs constant evaluation, which does not exist. Such a type cannot be interned
correctly today.

## The syntax this table has to hold

Hadley, 2026-09-02. Pointers and references are C++'s: `T*`, `T***`, `T&`, and
`T&&` will exist. Arrays are written like C++'s **and read unlike them**:

```
i32[10][5]      a 10 by 5 matrix
i32[10][5]*     a pointer to that matrix
i32[10]*[5]     an array of five pointers to i32[10]
```

Every postfix applies in written order, each wrapping what came before, which is
what `Parser::parse_type_postfix` already does. The type table copies the
nesting the parser built and adds nothing of its own.

**`T&&` is an rvalue reference** — Hadley, 2026-09-02, answering the trap this
record had left open. It is C++'s meaning and **not** two references, so the
precedent standing next to it is actively wrong: the parser deliberately reads
`**` as two pointers, and writing `&&` the same way would silently produce
`T&&` meaning `(T&)&`. It needs its own node kind and its own `TypeKind`. It
does not parse today.

## Decided: an unsized array is sugar for `Array<T>`

Hadley, 2026-09-02. `T[]` with no length is **not** a type of its own: it is
written form for `Array<T>`, and the sugar composes with everything else in the
order the postfixes were written.

```
i32[]           Array<i32>
Foobar*[]       Array<Foobar*>
Foo[][]         Array<Array<Foo>>
```

So `TypeKind` keeps `TYPE_ARRAY` for the **sized** form only — `i32[10]`, whose
length is part of its identity — and an unsized one desugars to a `TYPE_NAMED`
before it reaches the table. The `NO_LENGTH` sentinel stops being a length and
becomes a signal to the builder that this is the sugar.

**Open, and it blocks the implementation: where `Array` comes from.** Desugaring
needs the name `Array` to resolve to a declaration, and today every name a type
uses has to be in scope by a written import (record 0008 — nothing is implicit,
not even by a module's last segment). Two answers are possible and they are
different languages:

- **A prelude**: the compiler knows `Array` the way it knows `i32`, and the
  declaration comes from a module every compilation loads. That is a new
  mechanism and touches records 0008 and 0010.
- **It must be in scope**: `i32[]` is an error in a file that did not import
  the module declaring `Array`, exactly as `Array<i32>` would be. Nothing new,
  and a surprising error message for a piece of syntax that looks builtin.

Until that is answered the code keeps `i32[]` as an unsized `TYPE_ARRAY`, which
is the shape the sugar replaces and not a competing decision.

## What this record is not

It is the **representation**, and that is all. Three things the word "types"
usually implies are outside it, and two of them have no agenda item at all:

- **Inference.** `let a = 1` writes no type; Hadley confirmed on 2026-09-02
  that the language has inference and that `a` is an `i32`. Assigning types to
  expressions is a phase that does not exist and is not on the agenda.
- **Checking**, which needs 1.13, and 1.13 is undecided.
- **Layout** — size and alignment — which code generation will want, and which
  is where record 0007's by-value type cycle turns from allowed into impossible
  and has to be reported by somebody.

## Rejected

**A sibling chain for the arguments**, keeping the `Ast` idiom. It cannot be
interned; see above.

**A separate signature table.** A signature is a function type, and the return
being last costs one subtraction at the only place that cares.

**Probing the dependency's type table, as record 0013 does for names.** The
comparison is structural, so the probe would be a recursive cross-module walk at
every comparison instead of one walk at the import.
