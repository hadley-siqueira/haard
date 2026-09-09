# 0048 — Which pairs of types may convert

Status: **decided**, Hadley, 2026-09-09. Agenda 1.37, opened the same day by
[0047](0047-what-the-language-expects-of-the-programmer.md) and closed the
same day.

**The decision is that nothing changes.** No syntax, no entry, no code. This
record exists so that the next person to notice that `i8*` becomes a `File`
finds an answer instead of an oversight.

| | |
|---|---|
| Between **builtins**: the list stays **empty** | **decided**, Hadley |
| Into a **class**: every one-argument `init` stays a conversion | **decided**, Hadley |
| No marker is added — no `explicit`, no `implicit`, no conversion method | **decided**, Hadley |

## The question

Record 0047 left it, and it was Hadley's own:

> The way the compiler has of knowing when it is and when it is not, is to
> keep a table of types that make sense converted one into the other. [...]
> Now, does a `Counter` have the same semantic character as a classic numeric
> value? I believe not.

Record 0018's list has two halves and they are independent.

**Between builtins**, the list is empty and always was — rule 2, *no implicit
numeric conversion, in either direction*. That is why `let a : f64 = n` over
an `i32` is refused with no class anywhere near it.

**Into a class**, since [0046](0046-a-conversion-is-a-constructor-the-compiler-calls.md),
any class with an `init` taking exactly this type. Nobody declares it; it
falls out of having written the constructor.

## What was measured before deciding

**The whole standard library reaches exactly two class conversions.**

| | |
|---|---|
| `char*` → `String` | the intended one |
| `i8*` → `File` | the accident: that `init` exists to store the handle |

Every other one-argument `init` in `std/` is unreachable, and by two different
accidents of the rules. `List<T>`'s `init(@from : Array<T>&)` cannot be
reached because `Coercion::steps` reads a reference *before* `builds_from` is
asked, so an `Array<i32>&` arrives as an `Array<i32>` and the parameter type
no longer matches exactly. `Array<T>`'s `init(@from : T*, @count : i32)` has
two parameters. The copy constructors take `X&` and are shadowed by identity.

So the migration cost of *any* of the options was **one line**, in either
direction — mark `String` to keep it, or mark `File` to lose it. The choice
was never about cost. It was about which default is right for code that does
not exist yet.

**Two of sixteen casts would have disappeared** under lossless widening. The
`as` written in `std/` are `i32 → i64` and `u32 → i64` in `String.append`
(both lossless), and then: `i64 → i32` and `f64 → i64` (narrowing), `i32 →
u32` twice (sign), `i64 → f64` (widening by the letter and lossy in fact,
which is the Java bug record 0018 already names), and `symbol → char*` (a
representation, and the point of a symbol being its own type). Only the first
two would have gone.

## What was on the table and declined

**Opt-in — a marker saying an `init` is a conversion.** Nothing converts
unless the class's author says so; `close_it(h)` over an `i8*` becomes an
error naming `File(h)`. It is the option that most closely answers the
question as asked, since marking `init(@text : char*)` inside `String` *is*
declaring the pair `(char*, String)` kin — the per-constructor marker and the
per-pair table are the same statement written in different places.

**Opt-out — C++'s `explicit`.** Converting by default, the author marks what
must not. Nothing that compiles today would stop compiling.

**Lossless-only widening**, Zig's rule, which record 0018 had already
rejected *"only for now"*.

**Sign changes as well.** `-1` becoming `4294967295` without a word.

## The reversibility, which is the part that matters later

The three options are not equally cheap to defer, and the one that gets more
expensive with time is the one declined:

| adding it later | cost |
|---|---|
| lossless widening | **breaks nothing** — record 0018 says so, and it still holds |
| opt-out (`explicit`) | **breaks nothing** — it only refuses where a marker is added |
| opt-in (`implicit`) | **breaks every conversion that is not marked** — one line in `std/` today, and more with every class written |

So two of the three doors stay open at no cost, and the third narrows. That is
not an argument for reopening it; it is the fact a future session needs in
order to know what it is choosing between.

## Why this is coherent, and where it is not

It is coherent with [0047](0047-what-the-language-expects-of-the-programmer.md).
A conversion is a permission, not a refusal, and 0047's rule is about
refusals — but the stance behind it is the same: the language does not add a
keyword so that the compiler can second-guess a constructor the author wrote
on purpose. Where two conversions collide the compiler still says *I cannot
tell which*, and the answer is `send(Path(where))` — which is the diagnostic
0047 holds up as the model.

Where it is **not** coherent, and this is written down rather than smoothed
over: the question was raised because `i32 → Counter` and `i8* → File` looked
wrong, and neither is now refused. What the decision says is that they are the
author's to avoid, not the compiler's to prevent — the same answer 0047 gave
about the lost write and the dangling reference. The example Hadley gave as
obviously sensible, a `double` coming from an `int`, also stays refused.

## What to do when this comes back

It will come back, in one of two shapes, and each has an answer here:

- *"A class is converting and nobody asked for it."* That is this record. The
  fix is `explicit` (opt-out), it is additive, and the cost of the accident so
  far is one `init` in `std/file.hd`.
- *"Why do I have to write `as` to widen an i32?"* That is record 0018 rule 2,
  reaffirmed here. The fix is lossless widening, it breaks nothing, and it
  would have removed two casts from the standard library.

Neither is a bug. Both are this record.
