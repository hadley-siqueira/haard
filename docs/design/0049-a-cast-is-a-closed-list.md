# 0049 — A cast is a closed list, and there is one `as`

Status: **decided and built**, 2026-09-09. Hadley chose both forks, from the
survey in [notes/how-languages-do-casting.md](notes/how-languages-do-casting.md).

| | |
|---|---|
| `as` is **checked**, against a closed list | **decided**, Hadley |
| **One** spelling: no `truncate`, no `unsafe`, no `reinterpret` | **decided**, Hadley |
| A numeric conversion **stays** `as` and does not become `i64(n)` | **decided**, Hadley |
| A class **by value** is never reached by a cast, even up its own chain | **decided while writing it** |
| A pointer converts only to an integer it **fits in** — 64 bits | **decided while writing it** |
| `i64(n)`, record 0045's spelling, asks the **same list** | **decided while writing it** |

## What was wrong

`ExpressionTyper::cast` typed its operand for the record's sake, gave back the
written type, and checked **nothing**:

```cpp
u32 ExpressionTyper::cast(u32 scope, u32 node) {
    type_of(index, scope, first_child(node), INVALID_TYPE);

    return builder.build(index, scope, second_child(node));
}
```

So `as` was C's cast — every pair written was accepted — with the difference
that the backstop was g++:

```
let z = pt as i32          # hdc: fine

z.cpp:58:31: error: invalid cast from type 'h0_1_Point' to type 'int32_t'
```

A mangled name, a line nobody wrote, in a file nobody read. That is the shape
this project has now found ten times, and the one record 0037 exists to
prevent one layer over.

## The list

It fits in a sentence: **a number to a number, a pointer to a pointer, a
pointer and a whole number either way, up or down a chain of bases through a
pointer or a reference, and a symbol to a `char*`.**

```haard
let widened = n as i64          # a number to a number, either direction
let truncated = f as i32        # and losing whatever it loses
let bytes = p as i8*            # a pointer to a pointer
let address = p as i64          # a pointer and a number it fits in
let up = circle as Shape*       # up a chain
let down = shape as Circle*     # and down it, unchecked
let by_reference = shape_ref as Circle&
let text = s as char*           # record 0041's one way across
```

and what it refuses, each by name:

```haard
let a = point as i32       # there is no cast from Point& to i32
let b = n as Point         # there is no cast from i32 to Point
let d = s as i32           # there is no cast from symbol to i32
let e = point as Shape&    # there is no cast from Point& to Shape&
let g = f as char*         # there is no cast from f64 to char*
let i = solid as Shape     # there is no cast from Circle to Shape
let j = p as i32           # there is no cast from char* to i32
let m = i32(p)             # the same, in record 0045's spelling
```

That last one is the rule written while building it: **a class by value is
never reached by a cast, even up a chain it really is on.** Casting one would
copy the base part and discard the rest, which is the slicing record 0018
refuses at every other place a value is given to something. Through a pointer
or a reference it is allowed, because nothing is copied.

## Why one spelling and not several

The survey's other coherent answer is C++'s and Zig's: name the risk, so a
reader sees which casts overruled the type system and `grep` finds them.
Hadley chose one spelling. What that costs is written down: `as` says the
same word for truncating an `i64` and for reading one address as another kind
of thing.

What it buys is that nothing had to be migrated. Twenty of the twenty-one
distinct pairs written in the whole repository are between numeric builtins;
under a named-risk rule, twenty of twenty-one lines would have had to say
which species they were.

The same argument settled the second fork. Record 0045 already made `i64(n)`
work, so Swift's split — a conversion is a **constructor**, because it makes a
new value, and `as` only changes what the compiler calls what you already
have — was available for free. It was declined because it would have moved
about twenty sites in `std/` and left two spellings meaning one thing.

Note what this leaves: **`i64(n)` and `n as i64` both compile and are the same
C++.** Record 0048 keeps the first, this record keeps the second, and neither
is going away.

## Where the list came from

Measured, not imagined. `ExpressionTyper::cast` was instrumented to dump every
`(from, to)` pair, and the compiler was run over `examples/`, all thirteen
programs of `tests/programs`, and every case of nine suites. **Twenty-one
distinct pairs**, of which fourteen are lossless widening, four are narrowing,
one is a sign change, one is `i64 → f64` (widening by the letter and lossy in
fact), and one is `symbol → char*`.

No cast between classes, no cast up or down a hierarchy, and no pointer cast
appears in real code at all — the one pointer pair, `i32* → i32**`, is in a
**parser** case whose subject is the grammar. So the list has two halves: what
the repository writes, and the pointer work a compiler written in Haard will
need and no case had written yet. Nothing that compiled before this stopped
compiling.

## Why this does not contradict record 0047

[0047](0047-what-the-language-expects-of-the-programmer.md) says the compiler
refuses when it cannot decide, never when it disapproves. Refusing `n as
Point` is the first kind: there is no path from an `i32` to a `Point` and the
compiler knows it, the same way *no 'takes' takes these arguments* is a fact
and not a guess. It is not the compiler deciding a downcast is unwise — a
downcast is on the list, unchecked, exactly as the programmer wrote it.

## What was found while building it

**A caret aimed at a written type lands on the first word of the file.** The
diagnostic pointed at `second_child(node)`, the type, and three of the seven
refusals reported at `class Shape:` on line 16. A composite type node carries
token 0 — which `ExpressionTyper::name_of`'s own notes already say — so the
report points at the **`as`**, which the parser hangs on the cast node itself
and which is a real token every time.

**The hole was still open inside the list's own first draft.** A pointer to
*any* whole number was allowed, on the reasoning that Haard has no target
model so the width is the author's business. Then `p as i32` was compiled:

```
z.cpp:6:31: error: cast from 'char*' to 'int32_t' loses precision
```

Which is the same failure this record exists to close, one level in. A pointer
now converts only to an integer it **fits in** — `i64` or `u64`. That is
**sixty-four bits assumed**, and Haard has nowhere to write the assumption
down: there is no target, no word size, and the emitter names its integers
after `<cstdint>` and lets the C++ compiler place them. `holds_a_pointer` is
the first line to change the day Haard emits for something narrower.

**And record 0045's spelling asks the same list.** `i32(p)` takes the builtin
branch of `ExpressionTyper::construction`, which asked nothing — so leaving it
would have made the list a suggestion, since `i32(p)` says exactly what
`p as i32` says. Both spellings now refuse the same pairs, and the case pins
them side by side.
