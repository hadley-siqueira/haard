# How other languages do closures

Written 2026-09-22, before deciding what a closure is in Haard. The decision is
record [0058](../0058-a-closure-captures-by-reference.md); what Haard already
had is at the bottom.

A closure is **code plus the environment it captured**, and every language
here answers the same three questions differently:

- **What is captured, and how** — by value or by reference, written or
  inferred.
- **Where the environment lives** — the stack or the heap, and who frees it.
- **What type the closure has** — one type per closure, dispatched
  statically, or one type per signature, dispatched through a pointer.

## C — none, and the environment is passed by hand

```c
int by_key(const void *a, const void *b, void *ctx);
qsort_r(items, n, sizeof *items, by_key, &ctx);
```

A function pointer and a `void *` beside it. Nothing is captured: the author
builds the context, keeps it alive and casts it back.

**Cost:** every library invents its own convention for where the `void *`
goes, and the cast back is unchecked. **What survives:** this is exactly what
every other model below lowers to.

## C++ — an anonymous class, and the capture list is written

```cpp
int total = 0;
std::for_each(v.begin(), v.end(), [&total](int x) { total += x; });
auto f = [p = std::move(p)](int x) { return p->at(x); };
```

A lambda **is** a class the compiler writes: one field per capture and an
`operator()`. `[x]` copies, `[&x]` refers, `[=]` and `[&]` pick a default,
and `[p = ...]` initialises a field with anything. The environment is that
object, wherever it was put — usually the stack.

Every lambda has its **own unnamed type**, so passing one on needs either a
template (monomorphised, inlined, free) or `std::function` (the type erased,
an indirect call, and possibly a heap allocation). A lambda capturing nothing
converts to a plain function pointer.

**Cost:** `[&]` on a lambda that outlives its scope dangles, and nothing says
so. **What survives:** the class the compiler writes, and the split between a
unique type and an erased one.

## Rust — the same class, and the capture is inferred

```rust
let mut total = 0;
v.iter().for_each(|x| total += x);          // captures &mut total
let f = move |x: i32| x + offset;            // captures offset by value
fn apply(f: impl Fn(i32) -> i32) { ... }     // monomorphised
fn apply(f: &dyn Fn(i32) -> i32) { ... }     // fat pointer: data + vtable
```

The model is C++'s, but nothing is listed: each variable is captured in the
weakest mode its use needs (`&`, `&mut`, or by value), and `move` forces all of
them by value. Which of `Fn`, `FnMut` and `FnOnce` a closure implements falls
out of what it does with what it captured. The borrow checker is what makes
the inferred references safe.

**Cost:** three traits to understand before a callback can be stored, and
`Box<dyn Fn>` for anything that goes up the stack. **What survives:** inferred
capture, and `&dyn Fn` — a pair of pointers — as the erased form.

## Zig — none, on purpose

```zig
const Ctx = struct { total: *i32,
    fn add(self: Ctx, x: i32) void { self.total.* += x; } };
forEach(items, Ctx{ .total = &total });      // context: anytype
```

Zig refuses closures because a capture hides an allocation and a lifetime,
and Zig's rule is that neither may be hidden. The idiom is C's with a type:
a struct holding what would have been captured, passed as `anytype` and
monomorphised by `comptime`.

**Cost:** every callback is a declared struct. **What survives:** the argument
that where the environment lives must be answerable by reading the code.

## Swift — boxed variables and reference counting

```swift
var total = 0
items.forEach { total += $0 }                        // non-escaping
func later(_ f: @escaping (Int) -> Void) { ... }     // may be stored
```

A captured variable is moved into a heap box that both the function and the
closure share, kept alive by ARC. A closure has one type per signature,
`(Int) -> Void`. **`@escaping`** marks a parameter whose closure may be
stored; one without it is known to go only down, and the compiler may keep
its environment on the stack. A body of a single expression gives it back
with no `return`.

**Cost:** ARC on every capture, and `[weak self]` to break the cycles it
makes. **What survives:** escaping and non-escaping as the line that decides
where the environment lives, and the single-expression rule.

## Go, Java, C#, JavaScript — the collector answers everything

```go
total := 0
each(items, func(x int) { total += x })     // total escapes to the heap
```

A captured variable is promoted to the heap (Go's escape analysis, C#'s
display class, JavaScript's scope object) and the garbage collector frees it.
Java alone captures by value, and makes that visible by requiring the
variable to be *effectively final*.

**Cost:** needs a collector. Nothing here survives for a language without one.

## Pascal and Algol — nested procedures that only go down

```pascal
procedure Sum(var items: array of Integer);
var total: Integer;
  procedure Add(x: Integer); begin total := total + x end;
begin Each(items, Add) end;
```

A nested procedure reaches the enclosing one's locals through a **static
link**, a pointer to its frame. It may be **passed as an argument** and never
returned or stored — the *downward funarg* — so the frame always outlives it,
by the shape of the language. GCC's nested functions are the same thing, with
a trampoline on the stack.

**Cost:** no callback can be stored. **What survives:** capture by reference
with the environment on the stack is safe, with no collector and no borrow
checker, for exactly the uses that only go down — `each`, `map`, `filter`,
`sort`.

## Side by side

| | capture | environment | type | passed on as |
|---|---|---|---|---|
| C | none, by hand | wherever the author puts it | function pointer + `void *` | both, side by side |
| C++ | **written** list | stack, in an unnamed class | unique | template, or `std::function` |
| Rust | **inferred**, `move` forces value | stack, in an unnamed class | unique + `Fn` traits | `impl Fn`, or `&dyn Fn` / `Box<dyn Fn>` |
| Zig | none, on purpose | a declared struct | the struct's | `anytype` |
| Swift | by reference, boxed | heap, ARC | per signature | `@escaping` or not |
| Go / Java / C# / JS | by reference (Java: value) | heap, collected | per signature | anything |
| Pascal / Algol | by reference, static link | the caller's frame | nested procedure | **down only** |

Two models underneath: **a class the compiler writes** (C++, Rust, and Zig
by hand), and **variables promoted to the heap** (Swift and everything with a
collector). Haard has no collector, so only the first is available.

## What Haard had on 2026-09-22

- The parser accepts `|x| { ... }`, `|x: i32| -> i32 { ... }` and `|| { ... }`,
  and the symbol collector declares a closure's parameters. Nothing in
  `type_table/` or the emitter has ever seen `AST_CLOSURE`, so
  `let f = |x: i32| { return x + 1 }` then `f(1)` is *no 'f' takes these
  arguments*.
- The **type** `A -> R` already parses (`(i32, i32) -> bool`, `i32& -> void`)
  and `TYPE_FUNCTION` exists in the type table — as the type of a function's
  **signature**, used by overload resolution. No value has ever had one.
- A reference that outlives what it names compiles: record 0047 keeps
  `def leak : i32&` returning a local, on purpose.
- Records 0025 and 0034: the emitter does not borrow a C++ feature the
  language has no concept of, and an intermediate representation for another
  target is coming. A C++ lambda or `std::function` in the output would be
  both.
- No interfaces (1.16), so nothing like `Fn` can be declared; and a generic
  called without type arguments is not inferred, so a closure's unique type —
  which no one can write — could not be handed to a generic yet.
