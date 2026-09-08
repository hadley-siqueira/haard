# How other languages do enums

Written 2026-09-08, for the enum record. Haard has an `enum` that parses and
declares and whose members do not type (agenda 2.10's last hole but one), and
Hadley has said which direction it goes: **a tagged union, optimised to a plain
integer when it can be**. This is the ground that choice sits on.

The survey is organised around one fact that only shows up side by side:

> **Two different features wear the same word.** One is a *named set of
> constants* — C's `enum`, an integer with better names. The other is a *sum
> type* — a value that is one of several shapes, each carrying its own data.
> Languages that treated them as different features ended up with two of
> everything; languages that treated the first as a **degenerate case of the
> second** got one feature and paid nothing for it.

That second sentence is the whole reason Hadley's direction is the right one,
and the reason it costs less than it looks.

## The three families

| family | languages | what an enum is |
|---|---|---|
| **Constants only** | C, C++ (`enum`, `enum class`), C#, Go, Pascal, Ada | a named integer, and nothing else |
| **Constants, but the members are objects** | Java, Kotlin | a class with a fixed set of instances |
| **A sum type, constants are the empty case** | Rust, Swift, Zig, OCaml, Haskell, Scala 3, F# | one of several shapes, each with its own payload |

The first family always grows a second feature for the sum type — C++ got
`std::variant`, C got `struct { int tag; union { ... }; }` written by hand, Go
got interfaces. **Two features, and the second one is worse than the first.**

## The table

| language | payload | names scoped | backing integer | exhaustive `switch` | ↔ integer | methods | generic | recursive |
|---|---|---|---|---|---|---|---|---|
| **C** | no | **no**, they leak | yes, implementation-defined | no | implicit, both ways | no | no | n/a |
| **C++ `enum`** | no | no | `: uint8_t` since C++11 | no | implicit to int | no | no | n/a |
| **C++ `enum class`** | no | **yes** | `: uint8_t` | no (a `switch` warns) | **cast only** | no | no | n/a |
| **C++ `std::variant`** | **yes** | n/a, by type | no | `std::visit` must cover all | `index()` | no | yes | needs a box |
| **Rust** | **yes** | yes | `#[repr(u8)]` | **yes, an error** | `as` for fieldless only | **yes**, `impl` | **yes** | needs `Box` |
| **Swift** | **yes** | yes | raw values, fieldless only | **yes, an error** | `rawValue` | **yes** | **yes** | `indirect` boxes it |
| **Zig** | `union(enum)` | yes | **`enum(u8)`, explicit** | **yes, an error** | `@intFromEnum` | **yes** | yes | needs a pointer |
| **Go** | no (`iota` consts) | no, package level | it *is* an int | no | it is an int | on the named type | no | n/a |
| **Java** | per constant, fixed | yes | `ordinal()` | switch expressions only | `ordinal()`/`values()` | **yes**, and a body per constant | no | n/a |
| **C#** | no | yes | `: byte` | no | implicit both ways | extension only | no | n/a |
| **Kotlin** | no (`sealed` for that) | yes | `ordinal` | **`when` is, over `sealed`** | `ordinal` | yes | on `sealed` | yes |
| **Ada** | no | yes | **representation clause** | **yes, or `others`** | `'Pos` / `'Val` | no | no | n/a |
| **Pascal / Delphi** | no | no | ordinal | `case` needs `else` | `Ord`/`Succ`/`Pred` | no | no | n/a |
| **OCaml** | **yes** | yes | no | **yes, a warning by default** | no | no | **yes** | yes, free |
| **Haskell** | **yes** | yes | no | yes, a warning | `fromEnum` for fieldless | type classes | **yes** | yes, free |
| **Scala 3** | **yes** | yes | `ordinal` for simple ones | yes, over `sealed` | `ordinal` | yes | **yes** | yes |

## What each one is paying for

**C and C++'s unscoped `enum`** — an integer with names, and the names leak
into the enclosing scope, which is why every C codebase writes `TK_` in front
of everything. It is the cheapest thing that could work, it costs nothing at
run time, and it checks nothing: any int converts in, and a `switch` that
forgets a case says nothing. This compiler is written with it, and the `TK_`
prefix in `token.h` is the tax being paid.

**`enum class`** fixed the two worst parts — the names are scoped and nothing
converts implicitly — and fixed nothing else. No payload, no exhaustiveness.

**`std::variant`** is what C++ offers instead of a sum type, and it is a
library: the alternatives are addressed by **index or by type**, not by name,
so `std::get<0>` and `std::get<Circle>` are what you write instead of a name
you chose. `std::visit` is exhaustive only because a lambda must accept every
alternative. Every C++ codebase that wants a real sum type ends up writing the
tagged struct by hand — which is what this compiler does with `Type`: a `kind`
and fields that mean different things per kind.

**Rust** is the one to copy from, and what it gets right is that the simple
case is the degenerate case:

```rust
enum Colour { Red, Green, Blue }          // one byte, a plain integer
enum Shape {
    Circle { r: f64 },                    // a tag and a payload
    Rect { w: f64, h: f64 },
}
```

Both are `enum`. The first compiles to what C's enum compiles to. `match` is
**exhaustive or it is an error**, which is the feature that pays for itself
every time a variant is added: the compiler lists every place that has to
change. The cost is that an enum is as big as its **largest** variant plus the
tag, and that a recursive one needs a `Box` written by hand — `enum List { Cons(i32, Box<List>), Nil }`.

Its layout optimisation is the part worth stealing: **niche filling**.
`Option<&T>` is the size of a pointer, because a reference can never be null
and the null pattern is used for `None`. `Option<NonZeroU32>` is four bytes.
No tag is stored at all when the payload has a spare bit pattern.

**Swift** is Rust's design with a different bet on recursion: an enum is
value-typed and flat, and `indirect` is the word that makes one variant boxed:

```swift
indirect enum Expr {
    case number(Int)
    case add(Expr, Expr)
}
```

It also keeps **raw values** as a separate thing — `enum Code: Int { case ok = 200 }`
— so "an integer with names" and "a sum type" are the same declaration but the
integer half only exists when no variant has a payload. That is precisely the
optimisation Hadley described, spelled out in the language instead of left to
the back end.

**Zig** keeps them apart on purpose and makes the machine visible:

```zig
const Colour = enum(u8) { red, green, blue };   // the backing type is written
const Value = union(enum) { int: i64, text: []const u8 };
```

`union(enum)` generates the tag enum from the union, `switch` is exhaustive,
and `@intFromEnum` is the only way to an integer. A **non-exhaustive** enum is
written `enum(u8) { a, b, _ }`, which is the honest answer to "this comes from
C and may hold anything".

**Go** has no enum. `const ( Red Kind = iota; Green; Blue )` is a named integer
with no exhaustiveness and no payload, and the sum type is an interface — which
means dynamic dispatch, heap allocation, and a `switch v.(type)` that no
compiler checks for completeness. It is the cautionary column.

**Java** made an enum a class whose instances are fixed, which buys methods and
per-constant bodies, and costs an object and a reference. For sum types it
finally grew `sealed interface` plus records plus pattern matching in `switch`
(21) — three features to reach one.

**C#** is C's enum with scoped names and `[Flags]` for bit sets. It has no
exhaustiveness at all, and it cannot: any integer can be cast into an enum
type, so a `switch` always needs a default.

**Ada** is the one nobody copies and should: an enumeration is a first-class
scalar type with `'Pos`, `'Val`, `'Succ`, `'Pred`, ranges, arrays **indexed by
the enum**, and a representation clause that pins the values —
`for Colour use (Red => 1, Green => 2, Blue => 4);`. A `case` must cover every
value or say `others`. Pascal's `set of Colour` is the other idea from that
family worth remembering: a built-in bitset over an enum, which is what C#'s
`[Flags]` imitates by hand.

**OCaml** is where the representation trick comes from, and it is exactly
Hadley's:

```ocaml
type colour = Red | Green | Blue          (* immediate integers *)
type shape = Circle of float | Rect of float * float   (* boxed, with a tag *)
```

A **constant constructor** is an unboxed integer; a constructor with arguments
is a heap block whose header holds the tag. So a variant type with no payloads
*is* an integer, decided by the compiler and not by the author, and the code
that pattern-matches does not know which it got. **Haskell** is the same idea
with everything boxed and pattern matching that warns, rather than errors, on a
missing case.

**Scala 3** unified the two spellings the way Rust did, a decade later and on a
JVM: `enum Colour { case Red, Green, Blue }` and
`enum Option[+T] { case Some(x: T); case None }` are one keyword, compiling to
an integer-backed class in the first case and to sealed classes in the second.

## The three lessons

**1. The two features are one feature, and the simple case is the empty case.**
Rust, Swift, Scala 3 and OCaml all landed here from different directions; C++,
Java and Go all landed on two features and a worse second one. A language that
starts with the tagged union gets the C enum for free, and a language that
starts with the C enum never gets the tagged union without adding a second
thing.

**2. Exhaustiveness is what people actually buy an enum for.** Every language
in the sum-type family checks it, and every language in the constants family
does not — and the one thing every user of the constants family writes by hand
is a `default: unreachable()`. In this compiler that is 36 switches over
`AstNodeKind` and `TokenKind`, none of them checked by anything.

**3. Layout is the back end's business, and every language that says so
wins.** OCaml decides immediate-versus-block, Rust fills niches, Swift uses
spare bits. The author writes one declaration; what it costs is decided by the
compiler. The only language that makes the author write the representation is
Zig, and it does it because making the machine visible is Zig's whole point.

## What this leaves for Haard to decide

The direction is Hadley's and this note does not argue with it. What it does
say is that the design has five questions in it, and four of them have an
answer everyone agrees on:

| question | what the field says |
|---|---|
| One feature or two? | **One.** The payload-free enum is the degenerate case. |
| Who decides the representation? | **The compiler.** Plain integer when no variant carries anything; tag plus union otherwise. |
| Is a `switch`/`match` exhaustive? | **Yes**, everywhere it exists — and this is the feature, not a nicety. |
| Does it convert to an integer? | **Only when written**, and only for the payload-free case. |
| Recursion? | Rust and Swift make the author say `Box`/`indirect`; OCaml and Haskell box everything. **Open**, and it is the one place the answers differ. |

And two things Haard has that change the shape of the answer:

- **There is no pattern matching**, and no `switch` at all. An exhaustive
  `match` is a second feature to design, and without it a tagged union can only
  be read by asking which variant it is and then reaching for the payload —
  which is C's `struct { tag; union; }` with better syntax. The 36 switches in
  `src/` say this is worth deciding at the same time and not after.
- **Generics monomorphise** (record 0002), so `Option<T>` is a cloned
  declaration per `T` and its layout is decided per clone. That is Rust's
  position exactly, and it is what makes the payload-free optimisation
  decidable at all: a clone knows every variant's type.
