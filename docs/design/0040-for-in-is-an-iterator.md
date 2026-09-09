# 0040 — `for x in` is an iterator, one per container

Status: **decided** 2026-09-06, **built** 2026-09-08. The options were laid
out in [notes/what-for-in-means.md](notes/what-for-in-means.md), and what
building it found is at the end of this record.

| | |
|---|---|
| A container is walked by a **cursor it gives back**, not by indexing | **decided** |
| **One cursor class per container**, so each walks the way it is fastest | **decided** |
| The names are `iterator()`, `has_next()`, `next()` | **decided** |
| `x` is a **reference** to the element | **decided**, corrected 2026-09-06 |
| `for i in 0..10` becomes a plain `for`, with no `Range` class | **decided** |
| `..` takes its end **in** and `...` stops before it | **the scanner's**, and this record's examples were written the other way round |
| A fixed array **is** iterable, by its compile-time length | **decided**, corrected 2026-09-06 |

## What it lowers to

```haard
for x in xs:
    total = total + x
```

becomes, in a lowering that runs **after the types** (see *A fixed array is
iterable* below for why it cannot be the plain sugar pass):

```haard
let __c0 = xs.iterator()

while __c0.has_next():
    let x = __c0.next()

    total = total + x
```

The calls resolve like any others, and a container that answers none of them is
reported by name.

## Why a cursor and not an index

The index form is free — `while i < xs.length(): let x = xs[i]` needs no
library at all, because `Array` and `List` both have `length` and
`operator[]`. It was rejected for the reason it is tempting.

`List`'s `operator[]` **walks from the head**, which Hadley decided knowing it
is O(n) and calling it the author's to know. A `for` loop turning that into
**O(n²)** is the language making that mistake on the author's behalf, silently,
in the one construct written precisely because it is supposed to be the
obvious way to walk a thing.

## Why one cursor per container and not one generic cursor

A `Cursor<C, T>` over a `length()`/`at(i)` convention would be written once and
serve everything — and it would index, which is the same O(n²) on a list
wearing a different hat.

Hadley, 2026-09-06: *"one iterator per container, so that iterating can be as
efficient as it can possibly be."* So `ListCursor<T>` holds a `Node<T>*` and follows `next`,
`ArrayCursor<T>` holds a position, and each is as fast as its container can be.

## What a cursor holds

**The container**, not a copy of its insides. A cursor that copies `data` and
`size` freezes the array: an `add` during the loop is unseen, and a `reserve`
leaves the cursor holding a pointer that has been freed.

Reading through it needs no new syntax — `(*held)[i]` reaches the overloaded
`[]` through a `T*`, which is **measured and works**, so the cursor may hold an
`Array<T>*` and index it without naming an operator that has no name (record
0034).

## The range

`for i in 0..10:` becomes a plain loop and needs **no class**:

```haard
let __e0 = 10          # once, because it may have a side effect

let i = 0

while i < __e0:
    ...
    i = i + 1
```

`<` for `..` and `<=` for the inclusive form. The range is visible to the pass
as `AST_EXCLUSIVE_RANGE` / `AST_INCLUSIVE_RANGE` under the `in`, so this is
syntax and needs no type — which is as well, since `0..10` still has none.

## What `x` is

**A reference to the element**, and it needs nothing written to be one: `let x
= xs[0]` already binds a reference today, because `operator[]` gives back a
`T&` and a binding keeps it. Measured — the C++ is `int32_t &h0_4_x = ...` and
writing `x` writes the element.

An earlier draft of this record said *a copy*, and that was wrong twice over.

**It is wrong about Haard.** A copy of an element that **owns** something is
refused by record 0031 unless the class writes the copy `init`, so
`for x in <an Array of Owner>` would not compile at all — while a reference
walks it happily. And a copy of a `String` element allocates once per turn, in
the construct that exists to be the obvious way to walk a thing.

**And it is not what the languages that matter here do.** The split is by
semantics, not by taste:

| | what `for x in xs` gives |
|---|---|
| C++ | `auto x` copies, `auto& x` aliases — **the language makes you say** |
| Rust | `for x in v` consumes `v`; `&v` gives `&T`, `&mut v` gives `&mut T` |
| Java | a copy of the reference, so an object is aliased and a primitive is not |
| Python | the name is bound to the object, so a mutable element is aliased |
| Go | a **copy**, famously |
| C# | a copy, and **read-only** — it cannot be assigned |

The ones that copy are the ones where an element is small or where the language
is protecting a beginner. Haard has neither reason: it has value semantics with
an ownership rule that makes a copy an **error** for exactly the classes worth
iterating.

So `for x in xs: x = 0` **writes into the container**, which is what C++'s
`auto&` does and what a reader of `x` being a reference should expect. Whether
Haard later grows a way to ask for a copy is a question this record does not
answer.

## A fixed array is iterable

Hadley, 2026-09-06: *"it should be iterable, yes; the compiler should be able
to work out the fixed array's length at compile time and generate the right
loop."*

He is right and the earlier draft's reasoning was wrong. It said a syntactic
pass cannot see that `fixed : i32[3]` is a fixed array — true of the **pass**,
and irrelevant: the length is in the **type**, where record 0016 keeps it, and
the compiler has it.

What it costs is that the for-each lowering **cannot be a plain sugar rewrite**.
It has to know what it is walking, so it has to run where types exist:

```haard
for x in fixed:            # fixed : i32[3]
```
```haard
let __i0 = 0

while __i0 < 3:            # the length, read off the type
    let x = fixed[__i0]
    ...
    __i0 = __i0 + 1
```

### And that is now possible, which it was not

A pass that runs after the type phase creates declarations nobody has collected.
Record [0039](0039-a-clone-is-typed-when-it-is-made.md) is the machinery for
exactly that — `Instantiator` already registers a clone's symbols and now its
types are caught up on the spot, so the same two calls serve a lowering that
comes late.

So the for-each lowering becomes a **small phase of its own**, after the types,
choosing by what it is given:

| the receiver | what it writes |
|---|---|
| a class with `iterator()` | the cursor loop |
| a fixed array `T[N]` | an index loop with **N** written in |
| a range | a plain loop, bounds evaluated once |
| anything else | reported, by name |

The alternative was to keep the pass syntactic and have the compiler hand a
fixed array an `iterator()` of its own — record 0030's shape, applied to a type
instead of a module. It is smaller and it is more magic: a cursor class the
compiler invents for a builtin, to avoid reading a number it already has.

## What must exist first

Record [0039](0039-a-clone-is-typed-when-it-is-made.md) — done. Without it, a
cursor asked for on an array built from a literal (`for x in [1, 2, 3]`) fails
before any of this is reached, and so does the index form.

## What it will want

**Direct construction of a value.** `iterator()` cannot write
`ArrayCursor<T>(held)` — there is no way to construct a value with arguments,
so it must default-construct and then assign the fields. That gap is not about
iteration and this is simply where it bites.


## What writing it found, 2026-09-08

It is built, and everything above holds. Six things the writing decided that
the deciding had not.

### `..` is the **inclusive** one, and this record's examples are misleading

`scanner.cpp` maps `..` to `TK_INCLUSIVE_RANGE` and `...` to
`TK_EXCLUSIVE_RANGE` — Ruby's convention, and it was there before this record.
Every `for i in 0..10` written above therefore walks **eleven** times and not
ten, and the line saying "`<` for `..`" had the two spellings the wrong way
round.

The code follows the **tokens**, since a token kind that says `INCLUSIVE` is a
decision already taken and this record never argued with it. If the intended
language is the other one, the scanner is where it changes and
`for_each_lowerer.cpp` reads it off the kind — one line in each.

### It is not a phase in `Compilation::build`, and could not be

This record asked for "a small phase of its own, after the types". A phase
after the type phase is **too late**, and the reason is a single line of Haard:

```haard
for x in xs:
    let y = x
```

`y` is inferred by the walk over candidates, once, and the mark never goes
back. So if `x` has no type until a later pass, `y` is typed against nothing
and stays that way, silently. The loop has to be taken apart **before the
body's own bindings are inferred**.

It is, and by the one thing that already knew the order: the **loop variable's
candidate**. `SymbolCollector` points it at the loop itself — from a name there
is no way back to the sequence it comes out of — so `TypeCollector::type_of`
recognises it, calls the lowering there, and re-points the candidate at the
binding the lowering wrote. Candidates are walked in source order, so `xs` is
typed before the loop and `y` after it, and nothing else had to move.

`ForEachLowerer` is still a class of its own, in `src/haard/sugar/` beside the
other Ast → Ast pass. What it is not is a walk of the tree.

### The loop becomes a **block**, not a `while`

The cursor has to be declared before the loop, and a pass driven by a candidate
does not know the statement's parent. So the loop node is rewritten in place
into an `AST_BLOCK` holding both:

```
{
    let __c0 = xs.iterator()

    while __c0.has_next():
        let x = __c0.next()
        <body>
}
```

Which turned out to be the right shape for a second reason: the emitter writes
that block with **braces**, so two loops in one function are two cursors and
two `x`es that C++ never sees at once. The block also keeps the scope the
collector opened for the loop, because a scope is stamped with the node that
opened it and the node is the same one.

### The range and the fixed array lower to a C shaped `for`, not a `while`

This record wrote both as a `while` with the step at the end of the body. A
`continue` in the body would jump over that step and the loop would never end.
`for ; i < __e0; i = i + 1:` has the step in the head, where `continue` cannot
miss it — and the cursor form needs nothing, since its step is the `next()` at
the **top** of the body.

### A range walks with the type of its **end**

`for i in 0...xs.length()` over a `u32` length walks with a `u32`. Inference
reading the `0` alone would make `i` an i32 and the comparison a mistake about
types nobody wrote, so the end is typed first and handed down to the binding —
which is record 0018's rule about literals, applied to a loop.

### What is refused, and each exactly once

- `for key, value in pairs` — the shape the reference left unfinished. It
  parses and it binds both names; taking a value apart is agenda 1.23's
  question. Reported once, though the loop reaches the pass once per name.
- `for c not in xs` — a foreach as far as the parser is concerned, and it says
  the opposite of what a walk needs.
- a sequence that is not a class, a fixed array or a range — *"i32 is not
  something a `for ... in` can walk"*.
- a class that answers none of the three names — reported by the **call**,
  which is this record's own rule and the better sentence: *"Thing has no
  member named 'iterator'"*.

Inside a generic nobody instantiated, nothing is reported and nothing is
lowered: the clone is where it happens, with `T` bound, which is the rule the
statement checker and the emitter already follow.

### What it still costs

The sequence of a fixed array loop is **re-indexed every turn** — `fixed[__i0]`
— because binding it to a local would copy it. For a name that is free; for
`for x in f()[0]` it is a call per element. Nothing in the language can return
a fixed array by value today, so it cannot bite yet.

And `iterator()` is still written by hand in the library the way this record
said it would be: default-construct the cursor, then assign its fields. The
day a value can be constructed with arguments, those three lines become one.
