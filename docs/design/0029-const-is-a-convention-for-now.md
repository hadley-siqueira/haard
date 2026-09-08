# 0029 — `const` is a convention, for now

Status: **deferred**, 2026-09-05. Hadley, having been shown what it would take.
Written down so it is not proposed again, and so that when it comes back the
thinking is already done.

| | |
|---|---|
| `const` is **not** implemented, and stays a naming convention | **decided** |
| A string literal stays a mutable `char*`, and writing through one is still undefined | **known and accepted** |
| The semantics Hadley wants, and the design that delivers them | **written down below** |

## Where it stands today

`const` is a keyword that changes **nothing**. `const greeting = "hi"` emits
`char *greeting = "hi";` — byte for byte what `let` emits — so:

```haard
const greeting = "hi"

def takes : i32
    @p : char*
    p[0] = 'X'
    return 0

takes(greeting)     # compiles, and segfaults
```

There is no `const` in the type system at all: the eleven `TypeKind`s hold no
qualifier, and `let p : const char*` does not even parse. Haard is Python here
— a name in caps means the author meant it, and nothing enforces anything.

## The semantics Hadley specified

Recorded because they are the requirement, whenever this is picked up:

- `const foo = "hi"` — the bytes live in read-only memory and cannot be written.
- `let foo = "hi"` — a **mutable** `char*`, in Haard and in the emitted C++.
- `foo = "hi"` with no `let` — mutable too, so `foo[0] = 'a'` changes the `h`.

## The design that delivers them, in three separable pieces

**1. `const` as a qualifier in the type system.** A kind or a flag, a case in
each switch, `T*` → `const T*` on record 0018's closed list, and the emitter
writing `const`. Everything else depends on it.

**2. The storage rule.** `const foo = "hi"` → `const char *foo = "hi";`.
`let foo = "hi"` → a copy: `char foo_[] = "hi"; char *foo = foo_;`.

This has a hole, and closing it is what makes the design fall out. What of
`takes("hi")`, where there is no binding to copy into? If a literal stays a
`char*`, `takes` writes to read-only memory — today's bug with more ceremony.

**So a string literal is a `const char*`.** Then `takes("hi")` is refused at
compile time instead of segfaulting, `let foo = "hi"` copies and `foo` is a
mutable `char*` as specified, and `takes(foo)` works. The whole of the
semantics above falls out with no exception. It **amends records 0022 and
0023**: a literal is a `const char*`, and `String.init` takes one.

**3. Inferring which parameters a function modifies.** Hadley's idea: the
compiler works out for itself whether a function writes through a parameter,
and refuses a `const` argument to one that does, so nobody writes `const` on a
parameter.

Feasible — record 0019's `ResolutionTable` already records which candidate
every call means, so the call graph is derivable, and this compiler already has
three fixpoint loops.

But it is unsound wherever a **pointer escapes**: `&p`, storing `p` in a field,
handing it to a closure. A conservative analysis must call those "modifies",
and then the ordinary act of keeping a pointer makes a parameter mutable. It
would be conservative in the commonest case.

## Why it is deferred and not rejected

Pieces 1 and 2 alone are worth having: they kill the segfault and need **no
inference**, because the author writes `const` on a parameter the way C++ does.
Piece 3 stays possible afterwards as a **convenience**, and it is additive —
inferring that a parameter is not modified only ever loosens, which record 0018
says is the safe direction.

Hadley chose to spend the time on the standard library and the prelude first,
which `new T[n]` (record 0028) has just unblocked. That is a scheduling call
and not a judgement about the design.

## What it costs to wait

Named so nobody is surprised: writing through a string literal compiles and is
undefined. g++ says so on every one of them — *ISO C++ forbids converting a
string constant to `char*`* — and the emitted program segfaults rather than
being refused.
