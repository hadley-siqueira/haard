# How other languages write a cast

Written 2026-09-09, before deciding what `as` may do. The measured state of
Haard is at the bottom.

The question every language here answers is the same: **one cast that does
everything, or several that each say which risk is being taken?** C answered
"one" and every language after it has been reacting to that answer.

## C — one cast, all meanings

```c
(T) x
```

Numeric conversion, pointer reinterpretation, pointer to integer, dropping
`const`: one syntax for all of it, and no check beyond "the two types have
sizes". `(Widget *) 42` compiles.

**Cost:** you cannot grep for the dangerous ones, because they look exactly
like the harmless ones. Reading `(int) x` tells you nothing about whether a
value was truncated or a pointer was reinterpreted.

## C++ — four named casts, precisely because C had one

```cpp
static_cast<T>(x)       // conversions the type system knows: numeric,
                        // up or down a hierarchy, void*, user conversions
const_cast<T>(x)        // only adds or removes const
reinterpret_cast<T>(x)  // pointer to pointer, pointer to integer: "trust me"
dynamic_cast<T>(x)      // a downcast CHECKED at run time; null or throws
(T) x                   // C's, which tries static then const then reinterpret
```

The split is the whole point: `reinterpret_cast` is deliberately ugly and
greppable, so a code review can find every place the type system was
overruled. `dynamic_cast` is the only one that costs anything at run time, and
it is the only one that can fail safely.

**Cost:** four spellings, all verbose, and the C cast still exists and is what
people reach for.

## Rust — `as` is small on purpose, and everything else is named

```rust
x as u8                 // primitives ONLY: numeric, char<->u32, bool->int,
                        // pointer<->pointer, pointer<->usize
                        // narrowing TRUNCATES, silently
String::from(s)         // an infallible user conversion: the From trait
u8::try_from(n)?        // a fallible one: TryFrom, gives back a Result
mem::transmute(x)       // raw reinterpretation, and requires 'unsafe'
```

Rust refuses to let `as` reach user types at all. A conversion between two
structs is a trait implementation with a name, called explicitly, and a
conversion that can fail gives back a `Result` rather than a truncated value.

**Cost:** three mechanisms to learn, and `as`'s silent truncation is the one
piece widely regarded as a mistake — there are proposals to deprecate it.

## Zig — every risk has its own builtin

```zig
@intCast(x)        @truncate(x)       @bitCast(x)
@floatFromInt(x)   @intFromFloat(x)   @ptrCast(x)
@intFromPtr(x)     @ptrFromInt(x)     @enumFromInt(x)
```

The extreme of "name the risk". There is no general cast at all. Lossless
widening happens **implicitly**, so the only things you write are the ones
that can lose something — and each says exactly what it loses.

**Cost:** a large vocabulary, and the reader has to know which builtin does
what. The gain is that no cast is ever ambiguous about its danger.

## Java and C# — checked for references, explicit for numbers

```java
(int) longValue        // narrowing a primitive: explicit, truncates
(Circle) shape         // a downcast, CHECKED: ClassCastException if wrong
```

```csharp
(Circle) shape         // throws
shape as Circle        // gives null instead of throwing
checked { a + b }      // and numeric overflow can be made to throw
```

No pointer casts exist at all in either. Widening is implicit; narrowing is
written; a reference cast is a run-time check you cannot turn off.

**Cost:** the check is not free, and it needs run-time type information.

## Swift — a cast is about the static type; a conversion is a constructor

```swift
Int(x)          // a numeric conversion: a CONSTRUCTOR, not a cast
Int(exactly: x) // the fallible one, gives back an Optional
v as Shape      // guaranteed upcast, no run-time cost
v as? Circle    // downcast, gives back an Optional
v as! Circle    // downcast, traps if wrong
```

This is the cleanest split in the survey: `as` never makes a new value, it
only changes what the compiler calls the thing you already have. Making a
different value out of a value is construction, and it goes through `init`
like everything else.

**Cost:** two vocabularies, and `as!` is a loaded gun with short syntax.

## Ada — the unchecked one has to be instantiated

```ada
Integer (X)                              -- a checked conversion
function To_Int is new Unchecked_Conversion (...);  -- and the other kind
```

The dangerous cast is a generic you must instantiate and give a name to,
which makes it appear in the declarations of the file that uses it.

## The shape of the answers

| language | numeric | pointer reinterpret | downcast |
|---|---|---|---|
| C | `(T)x` | `(T)x` | `(T)x` |
| C++ | `static_cast` | `reinterpret_cast` | `dynamic_cast` (checked) |
| Rust | `as` | `as` / `transmute` | no inheritance |
| Zig | `@intCast`, … | `@ptrCast` | no inheritance |
| Java/C# | `(T)x` | — | `(T)x`, checked |
| Swift | `Int(x)` — a ctor | `unsafeBitCast` | `as?` / `as!` |
| **Haard today** | `as` | `as` | `as` |

Two axes come out of it:

1. **How many spellings.** One (C, Haard) through four (C++) to a dozen (Zig).
   The count tracks how much the language wants the *reader* to see the risk.
2. **Whether a numeric conversion is a cast at all.** Swift says no — it is
   construction, because it makes a new value. Everyone else says yes.

And one thing nobody does: **no language in this survey lets a cast between
unrelated types through unchecked and unnamed.** C is the closest, and C is
what C++, Rust and Zig were each reacting to.

## What Haard does today, measured

`ExpressionTyper::cast` types the operand for the record's sake and gives back
the written type. **Nothing is checked.** Every one of these passes `hdc`:

| written | `hdc` | `g++` |
|---|---|---|
| `n as i64`, `f as i32` | accepts | ok |
| `ci as Shape*` (up), `sh as Circle*` (down, unchecked) | accepts | ok |
| `p as i8*`, `n as char*`, `p as i64` | accepts | ok |
| `pt as i32` — a class to an integer | accepts | **refuses** |
| `i32(p)` — a pointer through record 0045's construction | accepts | **refuses** |

So Haard is C's answer, one cast for everything, with the difference that the
backstop is g++ — which reports in mangled names, about a line nobody wrote.

**The 21 distinct pairs written in the whole repository**, measured by
instrumenting the typer and compiling `examples/`, all 13 test programs and
every case in nine suites:

- **lossless widening (14):** `i8→i32` `i8→i64` `i16→i32` `i16→i64` `i32→i64`
  `u8→u32` `u8→i64` `u16→u32` `u16→i64` `u32→i64` `i32→f64` `f32→f64`
  `char→i32` `char→u32`
- **narrowing (4):** `i64→i32` `u64→i32` `f64→i64` `u32→i32`
- **sign change (1):** `i32→u32`
- **widening by the letter, lossy in fact (1):** `i64→f64`
- **not numeric (1):** `symbol→char*` — record 0041, and the only way across

Twenty of twenty-one are between numeric builtins. No cast between classes, no
cast up or down a hierarchy, and no pointer cast appears in real code — the
one pointer pair, `i32*→i32**`, is in a **parser** case whose subject is the
grammar (`a as i32 * b` cannot be told from a cast to `i32*`).

That is what exists. What does not exist yet is the compiler written in Haard,
where `src/` is full of pointers and of a base class reached through a
derived one.
