# 0059 — A generic is solved from its arguments

Status: **decided and built**, 2026-09-22. Hadley asked for it to unblock
`map`; the three rules below were decided while building it.

| | |
|---|---|
| A generic called with **nothing written** between `<` and `>` has its type parameters solved from the call | **decided**, Hadley |
| What is solved goes to the **same instantiation** a written `<...>` reaches | **decided while building it** |
| An argument that **has a type** decides first, a **number** written with nothing else to go on is what record 0018 makes it, a **closure** decides last, by what it gives back | **decided while building it** |
| A candidate that **cannot be solved is no candidate**, and the reader is told which parameter nothing said anything about, or which one two arguments disagree about | **decided while building it** |
| A generic **class** built with no arguments written — `Pair(1, 2)` | **done 2026-09-24**, record 0060 |

## What was missing

`f<i32>(3)` worked since record 0054 and `b.take<i32>(3)` since 0055, and
`f(3)` did not: the call was ranked against the **unbound** signature, where
an i32 does not match a `T`, and the answer was *no 'f' takes these
arguments*. There was no unification anywhere in the compiler.

It mattered for one method more than for all the others. `map` on a container
is a generic method whose own parameter, `U`, is what the closure gives back
— and `xs.map<i32>(|x| { x * 2 })` asks the reader to write down what the
compiler can see.

## Decision

### 1. The signature is laid over the arguments

Each parameter's type is compared with its argument's, structurally, and a
type parameter met for the first time is **bound** to what it is laid over:

| parameter | argument | binds |
|---|---|---|
| `T` | `i64` | `T = i64` |
| `T&` | `i32` | `T = i32` — record 0018 gives a value to a reference |
| `T` | `i32&` | `T = i32` — a reference is the thing it names (record 0035) |
| `T*` | `i64*` | `T = i64` |
| `Box<T>&` | `Box<f64>` | `T = f64`, read off the record that made the clone |
| `A -> B` | `i32 -> bool` | `A = i32`, `B = bool` |

Where no parameter is involved the answer is simply *yes*: whether the
argument really fits is the overload resolver's question, and it is asked
right after, against the clone.

### 2. In a fixed order

1. **An argument that already has a type.** It is the only one that says
   something for certain.
2. **A number written down**, and only for a parameter nothing else bound:
   `id(4)` is an `i32` and `id(2.5)` an `f64`, the types record 0018 gives a
   literal when nothing asks. `bigger(small, 2)` with `small : i64` binds T
   from `small`, and the `2` is then an i64 literal — it is never converted.
3. **A closure**, told the parameters it takes, which must all be known by
   then. What it gives back binds whatever is left. That is `map`:

   ```haard
   def map<U> : Array<U>          # inside Array<T>
       @f : T& -> U
   ```

   `T` is the Array's own and already concrete, so the closure is typed as a
   `T& -> ?`, and its one expression says what `U` is.

A closure whose parameters would still be unknown at that point is not
solved for; neither is one taking a generic class of the parameters
(`Array<T>&`), since naming that type would mean instantiating it before the
call is chosen.

### 3. The same instantiation as a written list

What was found is handed to `TypeBuilder::instantiate_written`, the entry a
written `f<i32>(3)` already reaches — so a solved call and a written one are
the same call from there on: the same clone, memoised by the same arguments,
typed the same moment.

### 4. What cannot be solved is not a candidate

Left in the list with its parameters unbound, `make()` — a generic whose `T`
appears in no parameter — matched it and gave back a `T` nothing would ever
bind, **in silence**. That was already so before this record, and this record
is where it stops. A generic that cannot be solved is dropped, and the call
says why, in the words that let the reader fix it:

```
error: nothing here says what 'T' is, so it has to be written: make<...>(...)
error: 'T' cannot be both i32 and i64
```

## What building it found

**A condition did not read through a reference.** `if b:` with `b : bool&` —
the variable of a `for b in` over bools, or what `each` hands a closure — was
*a condition must be bool, and this is bool&*, and so was an operand of `and`.
Record 0035 says a reference is the thing it names; the two places that ask
for a bool now read through one. It was there before this record and found by
the first program that mapped an Array into bools.

## What was rejected

- **Solving by trying every type.** There is nothing to try: the arguments
  are typed, and laying one type over another answers directly.
- **Converting a literal to agree with a typed argument.** Record 0018 has no
  numeric conversion, and here it has none either — the typed argument binds
  and the literal is asked to be it.
- **Letting a closure decide before a typed argument.** A closure is typed
  once (record 0058), so it has to be typed with everything the other
  arguments can tell it already known.
