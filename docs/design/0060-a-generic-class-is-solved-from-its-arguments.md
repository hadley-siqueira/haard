# 0060 — A generic class is solved from its arguments

Status: **decided and built**, 2026-09-24. Hadley asked for it; it is the part
record 0059 left open. The rules below were decided while building it.

| | |
|---|---|
| A generic **class** built with nothing written between `<` and `>` — `Pair(1, 2)`, `new Pair(1, 2)` — has its type parameters solved from its arguments | **decided**, Hadley |
| What is laid over the arguments is each **`init`** of the generic declaration, by record 0059's rules and in its order | **decided while building it** |
| Every `init` that solves is heard, and **they must agree**; two naming two different classes are the reader's to choose between | **decided while building it** |
| An `init` that does not answer to **this many arguments** is not asked | **decided while building it**, and record 0059's functions follow it too |
| A generic **enum** built from a variant with no arguments written — `Option.Some(3)` | **not done**; nothing asked for it |

## What was missing

`Pair<i32, f64>(1, 2.5)` worked since record 0045, and `id(4)` since 0059.
`Pair(1, 2.5)` was *'Pair' takes 2 type arguments, and 0 were written*, and
so was `new Pair(1, 2.5)`. From the standard library, that meant `List(xs)`
with `xs : Array<i32>` and `Range(1, 4)` had to be written `List<i32>(xs)` and
`Range<i32>(1, 4)`, which is the reader writing down what the compiler can
already see.

## Decision

### 1. The `init`s are laid over the arguments

A class has no signature of its own; what takes the arguments is an `init`.
So each `init` of the **generic declaration** is laid over them, exactly the
way record 0059 lays a function's signature, where its parameters name the
class's own `A` and `B`:

```haard
class Pair<A, B>:
    def init : void
        @a : A
        @b : B
```

`Pair(1, 2.5)` binds `A = i32` and `B = f64`, by 0059's three sources in 0059's
order: a typed argument first, a number by what record 0018 makes it when
nothing asks, and a closure last, by what it gives back. `Holder(|x| { x > 0 })`
with `init(f : i32 -> T)` is a `Holder<bool>`.

### 2. The solving says which class, never which constructor

What is found goes to `instantiate_written`, the entry a written
`Pair<i32, f64>` reaches, so the clone is the same one and memoised the same
way. The `init` is then chosen among the **clone's**, by the ordinary ranking
— the solving never picks one. So everything a written construction does, a
solved one does too: a literal takes its parameter's type, a closure is typed
against the chosen parameter, and the construction reaches a call.

### 3. Every `init` is heard, and they must agree

A class usually has more than one `init`, and each may say something different.
Two answers are possible and one was taken:

- **They must agree.** If every `init` that solves names the same class,
  that is the class. If two name two different ones, the reader writes which:

  ```
  error: this could be Wrap<i32*> or Wrap<i32>, so which has to be written: Wrap<...>(...)
  ```

- *Rank across the clones*, C++'s answer for its class template argument
  deduction: instantiate one clone per solving `init` and choose among all of
  their constructors at once. **Rejected**: it instantiates classes the
  program never uses, each of them emitted, to answer a question the reader
  can settle with one `<...>`. It is additive if it is ever wanted.

An `init` that cannot be solved, or that answers to a different number of
arguments, says nothing and is not asked. So `List(xs)` is the
`init(from : Array<T>&)` alone, and the `init` with no parameters does not
object.

### 4. What cannot be solved says why

The sentences are record 0059's, about a class:

```
error: nothing here says what 'T' is, so it has to be written: Box<...>(...)
error: 'T' cannot be both i32 and i64
error: no 'init' of 'Pair' takes these arguments
```

The first is also what a class with **no `init`** gets — an aggregate takes
nothing that could say what its parameters are. An argument that did not type
has already said so, and nothing more is said about the construction.

### 5. The arity range decides who is asked

`Pair(1)` against an `init` taking two used to be solved as far as it went —
`A` from the `1` and `B` from nothing — and came out as *nothing here says
what 'B' is*. That is about the wrong thing: nothing could have said it,
because the argument is missing. An `init` whose arity range (record 0012)
does not include the count written is now not asked at all, and the reader is
told that no `init` takes these arguments. A generic **function** follows the
same rule; before, it was only a count that was too **large** that skipped.

## What building it found

**A clone's fields had no type until the walk reached them.**
`let p = Pair<i32, i32>(1, 2)` followed by `let s = p.first` made `s` a
`<none>`, **in silence**, and it had been so since record 0045 made
`Pair<i32, i32>(1, 2)` writable. A clone made during inference is typed by
`TypeCollector::type_signature_now`, which gave the class its own type and its
methods their signatures, and **not its fields** — those waited for the walk's
next round, and a member access asked before then read the field's
`INVALID_TYPE`. Nothing reported it, because a member access with no type is
poison and says so to nobody. It is the shape of
*a clone made after a phase never gets that phase* again, and the fix is its
usual one: the fields are typed with the signatures, by what they wrote, and
the walk later types them to the same answer.

No test had read a field of a clone whose **first** instantiation was a
construction typed during inference. A clone an annotation made first is made
during the written pass, and the walk types it before anything asks.

## What was rejected

- **Solving from the fields**, as a struct literal would: a class is built by
  its `init`, and a class with an `init` may hold fields none of its arguments
  mention.
- **Ranking across clones** — see §3.
- **Guessing from the first `init` declared**: the order of declarations is
  not something the reader expects to change what a call means.
