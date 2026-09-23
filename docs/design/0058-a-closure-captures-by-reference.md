# 0058 — A closure captures by reference, and its value is a pair

Status: **decided and built**, 2026-09-22. Hadley's.

| | |
|---|---|
| A closure is an **environment** the compiler writes plus a **function** taking it | **decided**, Hadley |
| Every captured variable is captured **by reference**, and nothing is listed | **decided**, Hadley |
| The environment lives on the **stack** of the function that wrote the closure | **decided**, Hadley |
| Its value has the type **`A -> R`**, and is a pair: environment and function | **decided**, Hadley |
| A value of type `A -> R` follows **the rules of a `T&`** — record 0047's, so it may dangle | **decided** as a consequence |
| A body of **one expression** gives that expression back; a longer one needs `return` | **decided**, Hadley |
| A parameter with no type written takes it from the **`A -> R` expected there**, and with none expected it is an error | **decided**, Hadley |
| A named function is a value of type `A -> R` too | **decided** as a consequence |
| Closures that **go up** — stored, or given back — capturing by value on the heap | **deferred**, additive |
| A closure's own **unique** type, handed to a generic for free | **deferred**, waits on inferring a generic's arguments |

The comparison with other languages is
[notes/how-languages-do-closures.md](notes/how-languages-do-closures.md).

## What was missing

A closure parses, pretty prints and has its parameters declared, and nothing
after that has ever seen one: `AST_CLOSURE` appears in no file of
`type_table/` and none of the emitter. `let f = |x: i32| { return x + 1 }`
followed by `f(1)` is *no 'f' takes these arguments*. `each` and `map` on the
standard library's containers have waited on it since record 0040, whose own
note left `xs.each(|x| ...)` out for exactly this reason.

It is the one subject in the front end that needed a decision before any code.

## Decision

### 1. A closure is a class the compiler writes

The model of C++ and Rust, and the only one of the two models available to a
language with no collector. A closure becomes an **environment** — a struct
with one field per captured variable — and a **function** whose first
parameter is that environment, followed by the closure's own.

It is taken apart in the **front end**, into things the emitter already
writes: a struct and a function. **No C++ lambda and no `std::function`**, for
record 0034's reason — the output does not lean on a C++ feature the language
has no concept of — and record 0025's: the intermediate representation that
is coming for another target must be able to carry a closure without
reinventing it, and a struct plus a function is something every target has.

### 2. Everything is captured by reference

A captured variable becomes a **pointer to it** in the environment, and every
use inside the body reads and writes through it. So

```haard
let total = 0
xs.each(|x| { total += x })
```

changes `total`, which is what anyone reading it expects, and it matches
record 0040: the `x` of a `for x in` is a reference too. Nothing is listed —
unlike C++'s `[&total]` — because with one mode there is nothing to choose.

A captured **reference** is a pointer to what it names (record 0035), and a
closure written inside a method captures `this`, which is a pointer already.

### 3. The environment lives on the stack

It is a local of the function that wrote the closure, declared at the top of
that body and filled in **where the closure is written**. Taking addresses has
no side effect and a captured variable is always in view where the closure
names it, so — unlike record 0032's hoisting — nothing moves, and a closure on
the right of an `and` or in a loop condition is right.

### 4. Its value is a pair, and its type is `A -> R`

A value of type `(i32, i32) -> bool` is **two pointers**: the environment and
the function. It is C's function pointer with its `void *` beside it, and
Rust's `&dyn Fn`. It owns nothing, so it is copied freely and record 0031
asks nothing of it.

The type already parses, and `TYPE_FUNCTION` already exists as the type of a
**signature**. This record makes it the type of a **value** as well.

### 5. It follows the rules of a reference

A pair points into a stack frame, so a closure given back from the function
that wrote it, or stored in a field that outlives it, **dangles**. This is
**not a new rule**: it is exactly `def leak : i32&` returning a local, which
compiles today and which record 0047 keeps on purpose. Whatever Haard ever
decides about a reference that escapes, it decides about this.

In practice that makes the closures of this record the **downward** ones —
passed to `each`, `map`, `filter`, `sort` — which is the Pascal model, and the
one that is safe without a collector or a borrow checker.

### 6. One expression gives itself back

```haard
xs.sort(|a, b| { a < b })        # gives back a < b

xs.map(|x| {
    let y = x * 2
    return y + 1                 # required
})
```

Swift's rule. A body that is a single expression statement is that
expression's value; any other body is a function body, and needs `return` if
the type asks for a value. It is the parser's own example, `|x| { x + 1 }`,
made to mean what it looks like.

### 7. A parameter's type comes from where the closure goes

```haard
xs.each(|x| { total += x })      # x is i32&, from each's i32& -> void
let f = |x| { x + 1 }            # error: nothing says what x is
let g = |x: i32| { x + 1 }       # g : i32 -> i32
```

A closure written where an `A -> R` is expected — a parameter, a binding with
a type written, a `return` — takes its missing parameter types, and its return
type, from that. With nothing expected and a type missing, the compiler
**cannot decide**, which is the one reason record 0047 allows it to refuse.

Where the parameter belongs to a clone (`Array<i32>`'s `each`) the type is
already concrete. Where it belongs to a generic called without type arguments,
it waits on that inference, which does not exist yet.

### 8. A named function is a value too

```haard
xs.sort(by_length)
```

A function has no environment, so its pair holds a null one, and the function
half is a small adapter the compiler writes that drops it. An overloaded name
is picked by the `A -> R` expected, the way record 0018's resolution already
picks by the arguments.

## What the C++ looks like

```cpp
struct h_fn_f2b6b13 {                        // i32 -> void
    void *env;
    void (*call)(void *, int32_t);
};

struct h0_closure_74_captured {              // what one closure captures
    int32_t *h0_7_total;
};

static void h0_closure_74(void *h_env, int32_t h0_8_x) {
    h0_closure_74_captured *h_captured = (h0_closure_74_captured *) h_env;
    int32_t &h0_7_total = *h_captured->h0_7_total;
    h0_7_total += h0_8_x;
}

int32_t h0_6_main() {
    h0_closure_74_captured h0_closure_74_env;
    int32_t h0_7_total = 0;
    h0_1_each(4, (h0_closure_74_env.h0_7_total = &h0_7_total,
                  h_fn_f2b6b13{&h0_closure_74_env, &h0_closure_74}));
    ...
}
```

Every captured name is bound inside as a C++ reference **under its own
name**, so the body is emitted exactly as it would be outside. The environment
is a local declared at the top of the enclosing body and filled in where the
closure is written, with a comma expression — addresses have no side effects,
so nothing is hoisted, and a closure inside a condition or an `and` is right.
A call through a value goes through a helper per function type,
`h_fn_f2b6b13_call(f, 3)`, so the callee is evaluated once.

## What building it found

**The body could not be typed by the sweep.** A parameter takes its type from
where the closure goes, and for `xs.each(|x| {...})` on a line of its own that
is only known in the statement checker, a phase after the one that types
locals. Typed by the sweep, `let y = add_one(x)` inside the body was *no
'add_one' takes these arguments* — about an `x` that simply had no type yet.
So the sweep skips every scope inside a closure, and the typer asks the
collector for the body right after it has typed the parameters. Two typers can
reach a closure first and they share that collector, which is also what says a
closure is typed once.

**`(i32, i32) -> bool` was a function of one tuple.** The type grammar read the
bracketed list as a tuple type, and nothing had ever given a function type a
value, so nothing had noticed. A bracketed list in front of the first arrow is
now the parameters written together, flattened the way a variant's tuple
payload is — so `(A, B) -> C` and `A -> B -> C` are one type.

**A function type among parameters printed as if it were more of them.**
`i32 -> i32 -> void -> void` was `each`'s signature; a function type taken or
given by another is bracketed now, `i32 -> (i32 -> void) -> void`.

**An enum a function type names had no forward declaration.** The function
types are written right after the forward declarations, because a field may
hold one, and only classes and structs were declared there. An enum is
declared there now, on demand, in the shape record 0043 defines it.

## Not done, and none of it needs a decision

- a **method** given as a value — the emitter refuses it by name;
- an **overloaded** `def` given as a value — the name types to nothing, so the
  call it is given to reports that no overload takes it;
- `map`, which gives back an `Array<U>` and so waits on a generic called with
  no type arguments.

## Deferred, and why each is additive

**Closures that go up.** Stored in a field, given back, kept as a callback:
the environment would move to the **heap**, variables would be captured **by
value** (copied by `init`, record 0031), and the pair would gain a `destroy`.
That is `Box<dyn Fn>` or `std::function`. Nothing in this record has to change
for it; it is a second way to build the same pair.

**A closure's unique type, for free.** C++'s template and Rust's `impl Fn`:
each closure its own class, handed to a generic that the clone monomorphises,
with no indirect call. No one can write that type, so it needs a generic's
arguments inferred from the call — agenda item 2 of `docs/STATE.md` — first.

## What was rejected

- **By value by default** (Java). A callback that only goes down would copy
  every `String` it looked at, and `total += x` would change a copy.
- **A written capture list** (C++). With one mode there is nothing to write.
- **Swift's heap boxes.** They need reference counting on every capture, and
  Haard has none.
- **No closures** (Zig). Every `each` would need a declared struct, which is
  what the language already offers today and is what made `each` not worth
  writing.
- **The last expression of any body** (Rust, Ruby). A `def` needs `return`;
  a closure differing only when it is one line keeps the two readable as the
  same thing.
