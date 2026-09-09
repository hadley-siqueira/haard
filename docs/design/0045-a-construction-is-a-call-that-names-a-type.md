# 0045 — A construction is a call whose callee names a type

Status: **decided and built**, 2026-09-09. Hadley chose every fork below; this
records what was decided, what it compiles to, and what it deleted.

| | |
|---|---|
| The decision lives in **the call**, which branches to `initialisation` | **decided**, Hadley |
| The value is a **temporary written in place**, not a hoisted local | **decided**, Hadley |
| A class with `init`, `T()` with none, a **written generic**, and a **builtin** | **decided**, Hadley |
| Record 0023's two entries are **deleted in the same work** | **decided**, Hadley |
| An **enum** is refused by name: its variants are its constructors | **decided while writing it** |
| A cast converts **one** value, so `i32()` and `i32(a, b)` are refused | **decided while writing it** |

## What was missing

A class could only be built where the **compiler** decided to build one: a
written type at a binding (record 0037), or a `new` (record 0026). The author
could not say it.

```haard
let s : String = "abc"        # worked — record 0037
let p = new String("abc")     # worked — record 0026, and heap
let s = String("abc")         # error: no 'String' takes these arguments
```

That diagnostic was literally true and about a question nobody had asked.
`String("abc")` parses as an ordinary `AST_CALL`, the callee resolves to the
**class**, and `OverloadResolver::score` opens with

```cpp
bool callable = candidate->kind == SYMBOL_FUNCTION
             || candidate->kind == SYMBOL_VARIANT;
```

so a `SYMBOL_CLASS` scored −1, nothing survived the ranking, and the call was
reported as taking nobody's arguments.

## The decision

**A construction is a call whose callee names a type.** No node kind was
added, the parser was not asked to tell the two apart, and no Ast → Ast pass
runs: the difference is found in `ExpressionTyper::call`, before anything is
ranked, by asking what the name means.

```cpp
u32 made = construction(scope, node, callee, list, built);

if (built) {
    return made;
}
```

`TypeBuilder::type_symbol` was already the predicate — it is what a written
type uses to tell a class from a function of the same name — and it became
public for this. When it answers, the callee's two parts (the name, and the
generic arguments if any) go to `TypeBuilder::build_written_name`, which is
`build_named`'s own body reached without an `AST_NAMED_TYPE` node. That is
what makes a written generic instantiate here for free: `Pair<i32, i32>(1, 2)`
builds the clone before it looks for an `init`, exactly as a written type does
at a binding. Then `initialisation` — record 0026's, unchanged — picks the
`init`, ranks it, reports it, and writes the choice on the **call node**.

### The value is a temporary, and that is the whole point

Records 0032 and 0037 **hoist**: a template string and a bracketed literal
become a local declared before the statement that used them. Hoisting is
exactly why a bracketed literal still reaches only a binding — by the time a
call, a return or an assignment is reached it is a name and not a literal.

So this one does not hoist. The emitter writes `String(p)` in place, which is
C++'s own spelling and the same text `emit_conversion` was already writing
when the compiler decided a conversion on its own. It works in all four places
a value is given to something, and C++17's guaranteed elision means no copy
constructor is required — an uncopyable class constructs here too.

```cpp
h0_1_shout(const_cast<h2_1_String&>(static_cast<const h2_1_String&>(h2_1_String(h0_6_p))));
```

A temporary is an rvalue and C++ will not bind one to a plain `T&`; the cast
that gets around it was already written for record 0037 and needed nothing.

### A builtin callee is a conversion

`i32(x)` is the second spelling of `x as i32`, and Hadley chose it knowing it
collides. It cost the parser one branch, because a builtin is a **keyword**
and could not stand where a callee goes:

```
error: expected an expression, found 'i32'
```

It is read as the `AST_BUILTIN_TYPE` node it is and the postfix rule wraps it
into the call it was written as — **not** into an `AST_CAST`, which would have
been the same program in the tree and a different one on the page. The printer
is the parser suite's oracle, and it writes `i32(x)` back unchanged.

The `(` is required. A builtin on its own is a type and not a value.

## What it refuses, each exactly once

| written | said |
|---|---|
| `Colour(1)` | *Colour is built from one of its variants, so this needs the variant's name after a '.'* |
| `Box(1, 2, 3)` | *no 'init' of Box takes these arguments* |
| `Point(1, 2)`, no `init` | *Point declares no 'init', so it takes no arguments here* |
| `i32()`, `i32(1, 2)` | *'i32' converts one value, and 0 were written* |

The enum is its own refusal because record 0043 already made a variant
callable — `Action.Click(1, 2)` goes down the ordinary path and must keep it.
Saying that an enum declares no `init` would have been true and useless.

Inside a generic nobody instantiated, `T(x)` says **nothing**: `T` is a
parameter and not a class, so it has no `init`, and reporting that would
report a mistake about a body that is fine. Record 0002's rule, and the clone
asks the same question again with `T` bound.

## What writing it found

**`new Box<i32>(7)` had never worked**, since generics landed, and nothing had
written it. A clone instantiated in the module being walked has no signatures
yet — its turn comes on a later round of the same walk, and `catch_up` steps
aside for precisely that reason:

```cpp
if (walking.count(module_index) > 0) {
    return;
}
```

So `constructors_of` found the `init` and read `INVALID_TYPE` off it, `score`
returned −1, and the answer was *no 'init' of Box<i32> takes these arguments*
about an `init` that takes exactly those. `TypeCollector::type_signature_now`
is the half of `catch_up` this needed: one signature, built on demand. It is a
**request** and not a pass — `signature_of` reads the tree and interns, so the
walk builds the same answer again when its turn comes, and what must not
repeat (`require_default_construction`, which reports) stays with the walk.

## What it deleted

Record 0023's entry, and the reference one record 0035 added beside it. They
were the only entries on record 0018's list that named a **library** class,
and `Coercion::is_string` — which compared a declaration's name to the string
`"String"` — was the last place in the compiler that knew this class by name.
Both are gone, and so are the two helpers.

```haard
let p : char* = "xyz"

takes(p)             # was: passed in silence.  now: an error
takes(String(p))     # the conversion, said out loud
takes("xyz")         # unchanged — a written literal is record 0037
```

Record 0023's **ranking** survives it: a written literal reaching a
constructible class costs one step in `OverloadResolver::match`, which is what
keeps `char*` ahead of `String` when both overloads are in view. That was
already name-free.

The blast radius was measured before it was chosen and it was four lines: one
in `tests/programs`, one in each of two goldens' cases, and one comment that
had become false. `std/` and both examples needed nothing.

## Consequences

- Record 0018's list is a **language** relation from end to end. There is
  nothing on it the compiler has to know a class by name to do.
- Record 0023 is amended a second time: the named entry it described is gone
  and the mechanism it stood in for is writable.
- `T(args)` is what the `T&&`-shaped gap record 0040 ran into needed, and what
  `Hash<String, i32>()` needs to be spelled at all.
- Whether record 0018's list **composes** (record 0035's open point) is
  untouched, and it is now the only question left on that list.

## What was rejected

**A candidacy synthesised from each `init`**, so `choose` ranks a class like
any other overload set — record 0043's precedent, since a variant is callable
by having a signature. It is the more uniform shape and it was declined for
its cost: a candidacy would have to point at an `init` *inside* a class while
carrying the class's name, and an `init` returns `void` while a construction
returns `T`, so the one thing `choose` computes from a signature is the one
thing it could not use.

**An Ast → Ast pass** in the sugar lowerer. `String("abc")` and
`make_string("abc")` are the same node at that point, and the pass runs before
symbols exist — it would have had to learn to ask, or move.

**Hoisting**, as records 0032 and 0037 do. Consistent, and it would have
rebuilt the exact limitation this record exists to remove.
