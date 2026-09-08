# 0018 — Implicit conversion: a closed list, and nothing arithmetic in it

Status: **decided**, 2026-09-02. Agenda 1.13, which record 0012 opened and
which blocked overload resolution.

**Amended by [0023](0023-a-char-pointer-becomes-a-string.md), 2026-09-03**, in
the way rule 3 asks to be amended: one entry added to the list — `char*` to
`String` — and one addition to rule 5's ranking, a literal's distance from the
type it has when nothing asks. Nothing here is revoked, and rule 4 in
particular still stands: a library cannot make two of its types convertible.

| | |
|---|---|
| A literal has **no type** until its context gives it one | **decided** |
| **No implicit numeric conversion**, in either direction | **decided** |
| A **closed list** of coercions, none of them arithmetic | **decided** |
| **No user-defined conversion** | **decided** |
| Overloads rank by **inheritance depth** and nothing else | **decided** |

## Context

Record 0012 settled that Haard has implicit conversion and that it is
deliberately not C++'s, and left the logic open. It blocked overload
resolution, because resolution has to rank candidates by it.

A survey of twelve languages is in
[notes/how-languages-do-implicit-conversion.md](notes/how-languages-do-implicit-conversion.md).
The fact it turned up, and the one this record is built on: **how much implicit
conversion a language allows is how much complexity its overload resolution
inherits.** C++ has conversion, user-defined conversion and overloading at once,
and its error messages are the product of the three.

The argument that decided the direction is not about taste. **Loosening these
rules later is additive** — every program that compiled under them still
compiles. **Tightening them later breaks every program.** So the question was
not which rule is better but which rule allows changing one's mind, and only one
of them does.

## Decision

**1. A literal has no type until its context gives it one.** `1` is not an
`i32` that has to be converted into something else; it takes the type the
context asks for. `let n: i64 = 1` and `f(3)` where `f` takes a `u8` both work
with no conversion anywhere. A literal that does not fit is an error **about the
value** — `300 does not fit in u8` — and not about a type.

With no context, an integer literal is an `i32` and a float literal is an `f64`.

**2. No implicit numeric conversion, in either direction.** Not widening, not
narrowing, not signed to unsigned. `i16` to `i32` is written `cast`, like
anything else. This is the rule Java calls widening and gets wrong at
`long → float`, which is widening by the letter and lossy in fact; there is no
such exception here because there is no such rule.

**3. A closed list of coercions, and it fits in a sentence.** A reference or a
pointer to a derived type where a base is expected, `null` where any pointer is
expected, and **one level of dereference under a `.`**. All three are free at
run time and lose nothing. Nothing arithmetic is on the list and nothing will be
added to it without amending this record — which has now happened **twice**:

- [0023](0023-a-char-pointer-becomes-a-string.md) added `char*` to `String`.
  That entry is the one on the list that is **not** free at run time, and it is
  a library relation among the language ones.
- [0031](0031-what-copying-a-value-means.md) added **a value where a reference
  to it was expected**, and a derived value where a base's reference was. It is
  not a conversion at all — the parameter *is* the value — and nothing is
  sliced because nothing is copied. It was missing because nobody had needed
  it: 0031 is what sends a holder of something that owns memory towards a `&`
  parameter, and a `&` parameter could not be passed a value.

Nothing arithmetic is on the list still. And only in that direction: a
reference where a **value** was expected is a copy, which is 0031's subject.

The third is Hadley's rule of 2026-09-02 and it is a deliberate difference from
C++: **`.` reads a member of a `T` and of a `T*` alike**, and on a pointer it
means what `->` means. `->` is the explicit form and works **only** on a
pointer; writing it on a value is an error rather than a no-op. Both look
through exactly **one** level, so a `T**` has no members either way.

It is on this list rather than in the grammar because it is an implicit step the
compiler takes on the reader's behalf, which is what this list is for. Rust
carries the same kind of entry — its deref coercion — for the same reason.

**`this` is a `Class*`**, confirmed by Hadley on 2026-09-02. It is what a method
receives, and the rule above is what makes the choice invisible at the use site:
through a `.` a pointer and a value read identically, so `this.field` and
`this.method()` are written the same whichever it is. The place it becomes
visible is a written type — `let myself : Derived* = this` — and that is the
only kind of line a test can tell the two apart with.

**Upcasting by value is not on the list.** `Base` taking a `Derived` by value
copies the base part and discards the rest — C++ calls it slicing and it is one
of its classic bugs. It is an error here, not a conversion.

**4. No user-defined conversion.** C++'s converting constructors, C#'s
`implicit operator` and Nim's `converter` are the same feature, and all three
ecosystems advise against using it. With overloading in the language it is the
single thing that most damages a reader's ability to know what a call does: a
library can make two types convertible and change which overload *your* code
selects.

**5. Overloads rank by inheritance depth and nothing else.** With `f(Base&)` and
`f(Derived&)` called on a `Derived`, both match and the more derived wins. That
is a ranking, but over a **tree**, by an integer — not C++'s lattice of
conversion categories that are not comparable with each other. Two candidates at
the same distance are an **ambiguous call**, reported at the call, which is what
record 0012 already said.

### Single inheritance, and the question it leaves open

Hadley, 2026-09-02: **a class derives from one class and no more.** So "depth"
is a number along a chain, and rule 5 needs nothing else — with a graph it would
need a definition of distance that a tree does not.

He raised **interfaces** in the same breath and settled them the same day:
**Haard has none, and no syntax for them.** That is what makes rule 5 final
rather than provisional. A class implementing several interfaces would have
several ways up, and the single number would become a second axis with its own
tie-breaking — the shape of C++'s ambiguity between two conversion paths, which
is the shape this record spent its length avoiding. With one base and nothing
else, the ranking is a walk up a chain and cannot become anything harder.

## Consequences

- **Resolution is a comparison of `u32`s.** A call filters candidates by arity
  and compares lists of type indices. With no conversion there is no "better
  conversion sequence" partial order to define, implement or explain.
- **Type errors become local.** *expected `i32`, found `i64`* pointing at the
  argument, instead of *no matching function* followed by twenty candidates and
  why each failed.
- **Subtyping is not conversion, and the distinction is load-bearing.** A
  `Derived` *is* a `Base`; passing one where the other is expected converts
  nothing and moves no bit. Refusing it would make inheritance useless — every
  call would need a cast, which is to deny what the inheritance declared.
  Swift and Kotlin both refuse numeric widening and both accept upcasting, which
  is the evidence that these are two questions and not one.
- **The complexity moves to inference, and that is a better place for it.**
  Deciding what type the `1` in `f(1)` has is local to the expression, and its
  error is about a value.

## Rejected

**Lossless-only widening**, Zig's rule, which was the survey's other coherent
answer. It is defensible and it costs a distance metric in the resolution
ranking. It is rejected only for now and for one reason: it can be added later
without breaking a line, and the reverse is not true.

**C++'s ranking.** Record 0012 had already rejected it in advance.

**Rust's zero, taken literally.** Rust does not have zero either: it has a short
closed list of coercions about representation and never about value. This record
copies the shape of that list, not the claim that there is none.

## What this unblocks and what it does not

**Unblocked:** the second half of agenda 2.7, choosing among candidates by
signature.

**Still missing, and it is now the only thing in the way:** a call's arguments
have to have types before candidates can be compared, and **inference does not
exist**. It is not on the agenda and it needs to be.
