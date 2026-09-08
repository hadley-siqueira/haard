# 0035 — A reference is the thing it names

Status: **decided and implemented**, 2026-09-06. It **amends record
[0018](0018-implicit-conversion.md)'s list** for the second time — record
[0031](0031-what-copying-a-value-means.md) was the first — and it amends what
an assignment to a reference means.

Hadley, 2026-09-06: *"Implemente também T& com semântica parecida com C++."*

| | |
|---|---|
| A reference where a **value** is expected reads as what it names | **decided**, one step, and record 0031's question is asked |
| Assigning to a reference writes **through** it | **decided**, and this changes a recorded behaviour |
| Both of these are C++'s semantics, deliberately | **decided** |

## The hole

Record 0018's list had *a value where a reference was expected* — record 0031
added it, so that 0031's own advice, *hold it by reference*, could be followed.
It did not have the other direction, and the comment beside it said why:

> *Only in this direction. A reference where a **value** was asked for is a
> copy, and record 0031 has something to say about that.*

True, and it left a `T&` **unusable as a value anywhere**. Six places, found by
writing an `Array<T>` and reading it:

```
let a : i32 = xs.at(0)      error: expected i32, found i32&
let b = xs.at(0) + 1        error: a literal cannot be i32&
let c = takes(xs.at(0))     error: no 'takes' takes these arguments
let d = xs.at(0) == 10      error: a literal cannot be i32&
e = xs.at(0)                error: cannot assign i32& to i32
xs.at(0) = 99               error: a literal cannot be i32&
```

So giving a reference back from a function was pointless, and record
[0034](0034-an-operator-is-a-method-with-an-unwritable-name.md)'s
`operator[] : T&` — the only shape in which `a[i] = x` is possible at all —
could not have worked.

## Decision

**1. A reference reads as what it names, and it costs one step.** Not free,
because it *is* a copy: `Coercion::steps` asks record 0031's `may_be_copied`
first, so a class that owns something and says nothing about being copied
cannot be read out of a reference any more than it can be passed by value.

Costing one is what keeps an overload taking the reference ahead of one taking
the value when a reference is what was passed.

**2. Where a value is what matters, a reference is seen through.**
`TypeTable::value_of` is the whole of it, and it is asked in four places: the
two operands of a binary operator and the type handed down between them, the
left of a subscript, and the target of an assignment. One level and never a
walk, because `T&&` does not parse.

The literal is the reason the *expected* type has to be a value too, and not
only the answer: `xs[0] + 1` handed `i32&` to the right side, and the complaint
came back about the `1`.

**3. Assigning to a reference writes through it.** This is the change.

```haard
@b : Base&
@d : Derived&
b = d
```

used to pass — record 0018's list allows `Derived&` → `Base&` and an assignment
asked the list about the two references. Under C++'s semantics it assigns the
`Base` part of a `Derived` and discards the rest, which is the **slicing**
record 0018 keeps off its list on purpose, and which the binding
`let sliced : Base = d` two lines above it in the same test was already refused
for. **The two lines say the same thing now**, which they did not before.

The diagnostic names both sides by the **value** they are, so the message is
`cannot assign Derived to Base` — the slicing complaint — and not
`Derived& to Base&`, which names a pair the list allows.

## Consequences

- **`operator[] : T&` works**, and with it `xs[0]` and `xs[0] = 9` as one
  method. That was the point.
- **A reference variable cannot be rebound**, which was never written down as a
  decision and now is: `b = d` writes through `b`, and nothing in the language
  makes `b` name something else.
- **`tests/statement_checker/cases/the_coercion_list_is_not_only_for_a_call`
  changed**, and its comment says why. It gained `n = m` between an `i32&` and
  an `i32`, which is the shape `xs[0] = 9` takes.
- **`T&&` still does not parse**, and `value_of` leans on that.

## What is still open

**Whether the list composes.** `char*` → `String&` needs two entries — `char*`
→ `String` (record [0023](0023-a-char-pointer-becomes-a-string.md)) and then
value → reference — and record 0018's list is a list of **single** steps. Found
writing `shapes_and_text_by_template`, where the answer was to take the
parameter by value. It is record 0018's question and this record does not
answer it.

## A seventh place, 2026-09-08

The **address** of one. `&r` over a `T&` came out as a `T&*` — a type nothing
in the language can take — because `address_of` wrapped what it was given
instead of what that names.

It was found by asking whether this compiler could be written in Haard: every
table in it hands out a pointer into its own storage, `return &types[index]`,
and over an `Array<T>` whose `operator[]` gives back a `T&` that shape did not
type at all. One line, `pointer(value_of(inner))`, and C++ needed nothing —
`&` on a `T&` already gives a `T*` there, so only the type was wrong.

The case is the seventh block of
`tests/emitter/cases/a_reference_is_the_thing_it_names`, and
`every_expression_kind` pins the type.

## An eighth, 2026-09-08

`*r` over a `T*&`. `dereference` asked the type it was given instead of what
that names, so a capture of a variant carrying a pointer -- `case Node(held,
left, right)`, where `left` is a `Tree*&` -- could not be dereferenced:
*'*' needs a pointer, and this is Tree*&*.

Found the same way as the seventh: by writing a program. The seventh came from
asking whether this compiler could be written in Haard; this one from a binary
search tree whose node is an enum.
