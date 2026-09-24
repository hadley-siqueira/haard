# 0062 — A binding is a copy of what it is given

Status: **decided and built**, 2026-09-24.

Hadley, 2026-09-24: *"Copy, like C++. A reference only when it is written
explicitly (`let x : T& = ...`)."*

| | |
|---|---|
| A binding with **no type written** is a **copy** of what it was given, as C++'s `auto` is: given a `T&`, it is a `T` | **decided**, Hadley |
| A binding is a reference only when **written** as one: `let x : T& = ...` | **decided**, Hadley |
| The loop variable of a `for ... in` stays a **reference** to the element | unchanged: record 0040's, and the one exception |

## What was there

`let` took the type it was given, as it was. `operator[]` on an `Array<T>`
gives back a `T&` (record 0034), so `let x = xs[i]` was **another name for
the element**, and nothing in the source said so:

```haard
let i = line_starts[low]      # an i32&
i += 1                        # and this moved the table's own entry
```

Found by the bootstrap's `SourceFile` (`bootstrap/`), which computed every
column after the first wrong because of that one line. No record had decided
it. A type_table golden held `let seen = x` as `i32&`, which was the behaviour
written down, not a decision. Record 0035 says a reference is the thing it
names "with semantics close to C++'s", and C++'s `auto` copies.

## Decision

With nothing written, the binding's type is what it was given **read through
the reference**: a `T&` gives a `T`, and the value is copied into it. Record
0031 is asked of that copy, so a class that owns something and says nothing
about being copied is refused where its reference used to be accepted in
silence:

```
error: Owner cannot be copied, and this is given one
```

A reference is still one keystroke away, and now it is written where it is
meant:

```haard
let named : i32& = xs[1]
named += 100                  # writes xs[1]
```

### The loop variable

Record 0040 makes `x` in `for x in xs` the **element itself**, so that writing
`x` writes the container. The lowering turns the loop into
`let x = __c0.next()`, with nothing written, and the new rule would have made
it a copy. So the loop's binding asks to keep the reference
(`TypeCollector::written_or_inferred`'s `keeps_reference`). A binding inside
the loop is an ordinary binding: `let y = x` is a copy of the element.

## What changed besides

Three goldens moved, each to the new rule: `seen` in
`a_loop_variable_is_what_it_walks` and `hashed` in `every_expression_kind`
became values, and one emitter golden writes `int32_t first` where it wrote
`int32_t &first`, with the same exit status. `a_cast_is_a_closed_list` writes
its two reference bindings as references now, since what it pins is the type
of the **cast**. Nothing in `std/` depended on the old rule.

## What was rejected

- **Keeping the reference and recording it** — the other option Hadley was
  given. It is what Rust's `let` does with a `&T`, but Rust's references are
  spelled at every use and checked. Here nothing at the use says the name is
  an alias, which is how the bootstrap's bug went unseen.
- **Copying the loop variable too.** Record 0040 decided otherwise, and a loop
  that doubles every element is written for exactly that.

## The tests

- `tests/type_table/cases/a_binding_is_a_copy_of_what_it_is_given` pins the
  **type** of every shape: a method giving back a reference, a class, a
  parameter taken by reference, a cast to a reference, the refused copy of an
  owner, the loop variable, and each one written as a reference beside it.
- `tests/programs/cases/a_binding_is_a_copy_of_what_it_is_given` pins the
  **meaning**: what a write through a copy, a written reference and the loop
  variable changes. It reads no C++, so it holds under any lowering.

Turning the copy off fails both, and so does making the loop variable a copy.
