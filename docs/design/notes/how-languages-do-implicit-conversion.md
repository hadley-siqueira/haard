# How other languages handle implicit conversion

Written 2026-09-02, for agenda 1.13. Haard **has** implicit conversion and it is
deliberately not C++'s (record 0012); this is the ground to choose from.

The survey is organised around one fact that only shows up when you put the
languages side by side: **how much implicit conversion a language allows is how
much complexity its overload resolution inherits.** Haard has overloading
(record 0012), so this is not an idle comparison — it is the price list.

## The three families

| family | languages | what a call has to do |
|---|---|---|
| **Convert freely** | C, C++, Java, C#, Nim | rank candidates by *how good* each conversion is |
| **Convert never** | Rust, Go, Zig, Swift, Kotlin, Ada, OCaml | match exactly, or fail |
| **Convert nothing, adapt literals** | Go, Zig, Haskell, Swift, Rust (all of the "never" column) | the literal has no type until it is used |

The third is not a fourth family, it is what makes the second liveable. Every
language that refuses implicit conversion needs an answer to `x + 1`, and they
all give the same one: **a literal is not an `i32`, it is untyped until context
says otherwise.**

## The table

| language | int widening | int narrowing | signed ↔ unsigned | int → float | float → double | bool ↔ int | user defined | literals |
|---|---|---|---|---|---|---|---|---|
| **C** | yes | **yes, silent** | yes, silent | yes | yes | yes | no | typed, but promoted |
| **C++** | yes | yes (narrowing banned only in `{}`) | yes, silent | yes | yes | yes | **yes**, converting ctors + conversion operators | typed |
| **Java** | yes | no, cast needed | n/a, no unsigned | yes (**lossy and silent** for `long`→`float`) | yes | **no** | boxing only | typed |
| **C#** | yes | no | only where lossless | yes (lossy, silent) | yes | **no** | **yes**, `implicit operator` | typed |
| **Nim** | yes | no | no | no | yes | no | **yes**, `converter` | typed |
| **Go** | **no** | no | no | no | no | no | no | **untyped constants** |
| **Rust** | **no** | no | no | no | no | no | no | inferred, never converted |
| **Zig** | only lossless, same signedness | no | no | no | yes | no | no | **`comptime_int`** |
| **Swift** | **no** | no | no | no | no | no | no | literal protocols |
| **Kotlin** | **no** (deliberately dropped Java's) | no | n/a | no | no | no | no | typed by context |
| **Ada** | no between distinct types | no | no | no | no | no | no | universal_integer |
| **Haskell** | n/a | n/a | n/a | no (`fromIntegral`) | no | no | type classes | `fromInteger` + defaulting |

## What each one is actually paying for

**C** converts everything and says nothing. The famous consequence is
`if (x < sizeof(a))` with a signed `x`: the comparison turns unsigned and a
negative `x` becomes huge. There is no overloading, so nothing has to be ranked
— the whole cost lands on the reader.

**C++** is C plus user-defined conversions plus overloading, and the product of
those three is where its error messages come from. Its ranking is a lattice:
exact match beats promotion beats standard conversion beats user-defined
conversion beats ellipsis, applied per argument, and a call is ambiguous when
neither candidate is better on every argument. `explicit` exists because the
default was wrong often enough to need an opt-out. **This is the shape record
0012 already said Haard will not copy.**

**Java** kept widening and dropped everything else — no `bool`↔`int`, no
user-defined conversions. Its one famous wart is that `long → float` is implicit
and loses precision, which is widening by the letter and lossy in fact. Its
overload resolution runs in three passes (no boxing, then boxing, then varargs)
specifically to keep the common case from seeing the expensive rules.

**C#** is Java with `implicit operator` added back, and it is the cautionary
tale for user-defined conversions: a library can make two unrelated types
convertible and change which overload a call in *your* code selects.

**Nim's `converter`** is the same idea with the same result, and the Nim
community's own advice is to avoid it.

**Go** allows no conversion at all and pays for it with `int64(x)` written
everywhere — except in the place it would hurt most, arithmetic on literals,
because an untyped constant takes the type of its context. `var x int64 = 1`
works, `var x int64 = y` where `y` is an `int` does not. Go has no overloading,
so its resolution is trivial.

**Rust** is the strictest: no numeric conversion in either direction, ever.
What it does have is a short, closed list of **coercions** that are about
representation and not about value — `&String → &str`, `&[T; N] → &[T]`,
`&mut T → &T`, a function item to a function pointer. Every one is lossless and
none of them is arithmetic. Overloading exists only through traits, and a trait
method is chosen by the receiver's type, not ranked.

**Zig** is the most interesting middle: it allows exactly the coercions it can
prove **lossless and unambiguous** — `u8 → u16` yes, `u16 → u8` no,
`i32 → u32` no, `f32 → f64` yes — and gives literals a `comptime_int` type of
arbitrary precision that is checked to fit at the point of use. So
`const x: u8 = 300;` is a compile error about the *value*, not the type.

**Swift and Kotlin** both refuse widening on purpose and are the evidence that
programmers accept it: Kotlin dropped Java's implicit widening in a language
whose whole selling point was Java interop, and it was not the thing people
complained about.

## The three questions this puts to Haard

1. **Is a literal typed?** If `1` is an `i32`, then `let x: i64 = 1` needs a
   conversion rule and every arithmetic expression mixing widths needs one too.
   If `1` is untyped until used, most of the pressure for implicit conversion
   disappears — and Haard already has inference, which is the machinery an
   untyped literal needs.
2. **Is conversion allowed to lose anything?** "Lossless only" (Zig) is a rule
   with no exceptions to remember. "Widening" (Java) sounds like the same rule
   and is not, because `long → float` is widening and lossy.
3. **Can a user write one?** C++'s converting constructors, C#'s
   `implicit operator` and Nim's `converter` are the same feature, and all three
   ecosystems advise against using it. With overloading in the language, this is
   the single decision that most affects how hard a call is to understand.

The cheapest coherent answer, and the one the survey points at: **untyped
literals plus lossless-only conversion plus no user-defined conversions.** It
makes overload resolution nearly exact-match, which means ambiguity is rare and
diagnosable, and it is the combination Zig arrived at from C's direction while
Rust and Swift arrived at it from the other.
