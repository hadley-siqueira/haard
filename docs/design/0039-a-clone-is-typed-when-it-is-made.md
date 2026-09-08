# 0039 — A clone is typed when it is made

Status: **decided and implemented**, 2026-09-06. It is not a new rule so much
as record [0002](0002-generics-instantiated-in-the-importer.md) finishing what
it started, and it was found while designing `for x in`.

## The bug

```haard
let xs = [1, 2, 3]
let n = xs.length()     # error: no 'length' takes these arguments
let v = xs[0]           # error: no 'operator[]' takes these operands
```

`xs` is obviously an `Array<i32>`, and the compiler said so — the complaint is
not about `xs`, it is about the class.

## Why

The type phase is **two sweeps**, and the order between them is the whole of
it.

**Collect** takes what each declaration **wrote**. `let a : f64[]` is answered
here.

**Infer** takes what each binding was **given**. `let xs = [1, 2, 3]` is
answered here.

Typing that literal **instantiates a generic**: record 0002 clones `Array` into
a concrete declaration in the module that **declared** it — `std.array`, module
1 — while module 0 is the one being walked.

`let n = xs.length()` is also a binding with no written type, so it is answered
**in the same sweep, in the same module, a few candidates later**. Resolving
that call needs the clone's `length` to have a **signature**, and nobody has
typed the clone: its candidates exist and their signatures are `INVALID_TYPE`.
Overload resolution sees a candidate it cannot match and says so.

The clone gets its signatures when the sweep reaches **module 1**, later in the
same `for i in modules`. Module 0 has finished and has already reported.

**And that is why writing the type anywhere hid it.** `let xs : Array<i32>` —
or a parameter `@xs : i32[]&` on any function in the program — makes the clone
in the **collect** sweep, which repeats until nothing new is taken. Every test
case had such a helper, which is how this survived.

## Decision

**`TypeCollector::catch_up(module)`**: when `TypeBuilder` instantiates a
generic, it tells the collector to type the clone **now**, rather than on a
sweep that may already have passed the module asking the question.

Two things about it are not obvious and both cost a wrong attempt.

### It runs the **written** pass, not the inferred one

What a caller needs of a fresh clone is its methods' **signatures**, and a
signature is written. Typing its local bindings now would mark them done
**before what they depend on exists**, and nothing retries a candidate the mark
has passed — `let after = walk->next` inside `List`'s `destroy` came out
untyped.

### The builder has to be re-entrant

`TypeBuilder` keeps its subject — the module and its index — in members, and
this call now re-enters it. Without saving and putting them back at the public
entries, the outer build comes back pointing at another module. Two whole-program
cases fail without it.

**The typer does not need the same**, and it was written and then removed: the
catch-up runs the written pass, which is `TypeBuilder` work and never reaches
`ExpressionTyper`. A guard against a state the design excludes is not kept.

## What was tried and is wrong

**Nesting the two sweeps** — collect, infer, and round again until neither
grows. It does not work and it also **destroys**: a second collect over a clone
sets its *written* answer, and what a `let i = 0` writes is nothing at all, so
the loop un-types the very clones it was meant to finish. Repeating also cannot
help on its own, because the complaint was reported on the first pass and
nothing unreports it.

## The case

`tests/programs/cases/a_clone_made_while_inferring`, and **the order inside it
is the case**. Nothing there writes `Array<i32>` — not a binding, not a
parameter, not a return type — so the literal is what makes the clone, during
the pass that infers that very binding. Writing the type anywhere above hides
the bug completely.
