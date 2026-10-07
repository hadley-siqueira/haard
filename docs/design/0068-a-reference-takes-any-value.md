# 0068 — A reference takes any value

Status: **decided and built**, 2026-10-07. Hadley, asked why `bump((a, 2))`
refused its `2` when the parameter is an `(i32&, i32&)` -- *"não poderia criar
uma variável temporária para guardar o 2 e então ser passado para a
função?"* -- chose option (A) below: *"vamos de (A)"*. Amends record
[0035](0035-a-reference-is-the-thing-it-names.md), which said what a `T&` is
and not what it may be given, and follows item 6 of record
[0067](0067-a-tuple-is-a-struct-the-compiler-writes.md).

| | |
|---|---|
| A `T&` takes **any value** of its T: a place is referred to, anything else is held in a temporary the reference refers to | **decided**, Hadley |
| Inside an expression the temporary lives to the end of the statement; for a binding, to the end of the block | **decided**, Hadley |
| What the callee writes into a temporary is lost, silently | **decided**, Hadley -- the price of (A) |
| A number given to a `T&` costs one step more than given to a T, so `f(2)` between `f(i32)` and `f(i32&)` takes the value | **decided while building it** |

## What it was before

It had never been decided, and four places answered four ways:

| | before |
|---|---|
| `inc(2)`, `inc` taking an `i32&` | refused: *no 'inc' takes these arguments* |
| `inc(give())`, `give` giving back an `i32` | **passed hdc and g++ refused it** -- *cannot bind non-const lvalue reference* |
| `size("abc")`, `size` taking a `String&` | worked: the emitter built a temporary |
| `let p : (i32&, i32&) = (r, 2)` | refused: *expected (i32&, i32&), found (i32&, i32)* |

The second is the shape this project has found again and again: a program the
front end accepted and the C++ compiler did not.

## The options

**(A) A `T&` takes any value**, a temporary held for what is not a place.
Rust does it (`&mut 2`, `&mut give()`), and so did the string literal here.

**(B) C++'s rule**: a plain reference takes only a place, and `inc(give())` is
reported by hdc. C++ has it because of `void incr(int&); double d; incr(d);`,
where the implicit conversion made a temporary `int` and `d` never changed;
it then needed `const T&` to take a temporary at all. C# (`ref`) and Swift
(`inout`) also want a variable.

**Why (A)**: Haard has no `const` (record [0029](0029-const-is-a-convention-for-now.md)
deferred it), so `T&` is also the parameter that says *do not copy this*, and
refusing a temporary would make every such call name a variable first.
Record [0018](0018-implicit-conversion.md) has no numeric conversion, so
C++'s `incr(d)` trap does not exist here, and record
[0047](0047-what-the-language-expects-of-the-programmer.md) refuses only what cannot be decided
-- and this can.

```haard
inc(2)                          # 3, and the 2 is gone
inc(give())
bump((a, 2))                    # a is referred to, the 2 is a temporary
let x : i32& = 10               # refers to a 10 of its own until the block ends
let p : (i32&, i32&) = (a, 20)
size("abc")                     # as before
```

## How it is built

- The type phase knows a place (`ExpressionTyper::is_place`), so it decides:
  `ExpressionTyper::given_to` marks a value given to a `T&` that is not a
  place (`Module::hold_in_temporary`). It is asked at a call, a construction,
  a call of a function value, an overloaded operator, an element of a tuple
  and a binding with its type written.
- A literal asked to be a `T&` is asked to be the T (`literal`, and the string
  literal's construction), and is typed as the T after a call chooses.
- A tuple element given to a `T&` takes the reference whether or not it is a
  place, which record 0067 had kept as a value; and a comparison now hands
  the other side the **value shape** of a tuple of references, so
  `refs == (10, 20)` compares with two values and makes no temporary.
- The emitter: `emit_given` writes a marked value as
  `const_cast<T&>(static_cast<const T&>(value))` -- C++ binds a temporary only
  to a `const T&`, and it lives to the end of the full expression, which is as
  long as a call needs. A binding first writes each marked value as a local of
  its own (`hoist_temporaries`, `__rtN`), elements of a tuple before the
  tuple, so `let x : i32& = 10` does not dangle. Checked under AddressSanitizer.

**Not covered**: a `return` of a temporary from a function giving back a `T&`
compiles and dangles, as it does in C++ (record 0047). A literal on the right
of a **class's** operator is still typed against the class on the left before
the operator is looked up -- `Money(1) + 5` says *a literal cannot be Money*
-- which is older than this record and not changed by it.

Cases: `tests/emitter/cases/a_reference_takes_any_value` (255, a bit per
place), and `a_tuple_literal_takes_its_parameters_shape` and
`a_tuple_literal_argument_must_fit`, which item 6 wrote and this one
changed. Eight sabotages, each caught.
