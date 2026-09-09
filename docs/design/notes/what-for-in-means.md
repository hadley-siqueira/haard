# What `for x in xs` means — the options, for Hadley

Written 2026-09-06. Nothing here is decided.

## What is true today

`for x in xs:` **parses** and the tree is

```
for_each 'for'
  for_head
    in                      <- 'x in xs' is an ordinary binary node
      identifier x
      identifier xs
  block
```

The **symbol collector already declares the loop variables**, pointing their
candidates at the for-each node. The **typer has no case for it at all**, so
`x` types to nothing and the loop passes in silence. The **emitter has no case
either**.

So what is missing is the meaning, not the syntax.

## What Haard has to build it out of

- **Operator overloading** (record 0034) for `[]`, the binary arithmetic and
  comparisons, and `=`. **Not** unary `*` and **not** `++`.
- **No interfaces.** Single inheritance was settled 2026-09-02 and interfaces
  are named as a second axis nobody has asked for.
- **No sum types**, so no `Option` for a `next()` to give back.
- **Closures do not type**, so `xs.each(|x| ...)` is not available.
- **The Ast → Ast pass** (record 0025), which runs before the symbols are
  collected — so anything it writes resolves like ordinary source, and it
  cannot read a type.

That last point is the sharp one: **a purely syntactic lowering cannot know
what `xs` is.** Whatever protocol is chosen, the loop is rewritten into calls
by name and overload resolution does the rest.

## The options

### A. Index lowering — no protocol at all

```haard
for x in xs:
    total = total + x
```
becomes
```haard
let __i0 = 0

while __i0 < xs.length():
    let x = xs[__i0]

    total = total + x
    __i0 = __i0 + 1
```

**Needs**: `length()` and `operator[]`, which `Array` and `List` both already
have. **No library change at all.**

*For*: about forty lines in the sugar pass and nothing else. It is the smallest
thing that runs, and every program it compiles keeps working under any of the
others — `for x in xs` does not change shape.

*Against*: **O(n²) on a `List`**, because indexing one walks from the head. And
it only works for a thing that is indexed: no hash, no file, no generator.

### B. A cursor, by a name the language looks for

```haard
let __c0 = xs.iterator()

while __c0.has_next():
    let x = __c0.next()

    total = total + x
    ...
```

**Needs**: `iterator()` on the container, and `has_next()` and `next()` on
whatever it gives back. A cursor class per container — `ArrayCursor<T>`,
`ListCursor<T>` — which is library work and no compiler work.

*For*: O(n) on a linked list, and it works for anything that can be walked —
a hash, a file, a range, something computed. Three names the language looks
for, which is this project's habit (`init`, `destroy`, `to_string`, and record
0034's operators are all that shape).

*Against*: three names instead of two, and a class per container. A cursor
holding a `Node<T>*` also has to be told not to outlive its list, which nothing
in the language checks.

### C. C++'s `begin`/`end`

```haard
let __it = xs.begin()
let __end = xs.end()

while __it != __end:
    let x = *__it
    ...
    ++__it
```

**Needs** `operator!=` (have), unary `operator*` (**do not**) and `operator++`
(**do not**) — so two more operators on record 0034's table before it can be
written at all.

*Against*: it buys nothing over B here. C++'s iterators earn their weight from
algorithms taking pairs of them, and Haard has no such library and no plan for
one.

### D. The container walks itself, with no cursor class

```haard
let __s0 = xs.first()

while xs.valid(__s0):
    let x = xs.at(__s0)
    ...
    __s0 = xs.next(__s0)
```

**Needs** four names on the container and no new class: the state is an `i32`
for an `Array` and a `Node<T>*` for a `List`.

*For*: no cursor class to write, and the state is whatever the container finds
natural.

*Against*: four names rather than three, and every one of them is public
surface a user can call wrongly. B's cursor at least keeps its state to itself.

### E. A closure — `xs.each(|x| ...)`

Not available: closures parse and type to nothing, and this would make a loop
depend on the one part of the language that has never been designed.

## The two things none of them cover

**A fixed array and a pointer.** `for x in fixed:` where `fixed : i32[3]` has
no methods at all, and a syntactic lowering cannot see that it is a fixed
array.

**Answered 2026-09-06**: it is iterable, by the length the type carries, and
the lowering moves to after the type phase to be able to read it.

**A range.** `for i in 0..10:` is not a container. Two ways: special-case it in
the lowering, which is four lines and needs no library —

```haard
let i = 0

while i < 10:
    ...
    i = i + 1
```

— or make `Range` a class that answers whichever protocol is chosen. The first
is what most languages do internally anyway, and `0..10` has no type today.

## What `x` is

**Decided 2026-09-06: a reference.** This section first recommended a copy, on
the argument that it is what a reader expects. That was wrong for Haard: a copy
of an element that **owns** something is refused by record 0031, so
`for x in <an Array of Owner>` would not compile — and a `String` element would
allocate once per turn. The languages that copy are the ones where an element
is small or where the language is protecting a beginner; Haard is neither.

It also needs nothing built. `let x = xs[0]` already binds a reference, because
`operator[]` gives back a `T&` and a binding keeps it.

## Recommendation

**B**, and start by writing it for `Array` and `List` only.

A is tempting because it is free, and its own argument is what makes it a
trap: it works today because `Array` and `List` both have `[]`, and `List`
having `[]` is exactly the thing Hadley agreed was O(n) and the author's to
know. A `for` loop turning that into O(n²) silently is the language making the
mistake for them.

And the choice is **not permanent in the source**: `for x in xs` does not
change shape between A and B, so nothing a user writes today would have to be
rewritten. What A costs if abandoned is the compiler work, not the programs.

## Decided, 2026-09-06

**B**, and **one cursor per container** — Hadley: *"one iterator per
container, so that iterating can be as efficient as it can possibly be."* So `ListCursor<T>` follows
`next` and `ArrayCursor<T>` holds a position, and neither pays for the other's
shape. A single generic cursor over a `length()`/`at(i)` convention was on the
table and was rejected for the same reason A was: it indexes.

The cursor holds **the container** and not a copy of its insides, and
`(*held)[i]` reaches the overloaded `[]` through a pointer — measured, and it
works. `for i in 0..10` becomes a plain loop with no `Range` class.

**`x` is a reference**, corrected the same day: `let x = xs[0]` already binds
one, and a **copy** of an element that owns something is refused by record 0031
— so `for x in <an Array of Owner>` would not compile at all.

**A fixed array is iterable**, also corrected: the length is in the type, and
the compiler has it. What that costs is that the lowering cannot be a plain
syntactic rewrite — it has to run **after the types**, which record 0039 has
just made possible.

Written up as record [0040](../0040-for-in-is-an-iterator.md). **Not
implemented.**

## Side by side

| | compiler work | library work | a `List` loop | works for a hash |
|---|---|---|---|---|
| **A** index | ~40 lines | none | **O(n²)** | no |
| **B** cursor | ~40 lines | a cursor class per container | O(n) | yes |
| **C** begin/end | ~40 lines **plus two operators** | an iterator class per container | O(n) | yes |
| **D** four names | ~40 lines | four methods per container | O(n) | yes |
