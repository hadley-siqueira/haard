# 0034 — An operator is a method with a name no source can write

Status: **decided and implemented**, 2026-09-06. Hadley, asked whether `a[i]`
on a class should be an `at` convention or real operator overloading:

> *"On `at()` versus operator overloading: follow the proposed operator
> overloading syntax and implement it as operator overloading. When
> implementing it, the C++ emitter must not take advantage of C++ having
> operator overloading of its own. The Haard compiler should turn the overload
> into a function, and the C++ emitter should use that function."*

| | |
|---|---|
| A class declares `def operator[] : T&`, written against the word | **decided** |
| The method's **name** is `operator[]`, which no identifier can be | **decided** |
| The C++ is a **method call** and never a C++ operator overload | **decided** |
| `[]`, `+ - * / // %`, `== != < <= > >=` | **decided**, and the table is one line each |
| A class that did **not** overload an operator is an **error**, not a fall-through | **decided** |

## Why it was asked

`Array<T>` needs `xs[0]`, and `element_of` knew a pointer, an array, a list and
a hash and no class at all. The alternative on the table was an `at`
convention — the shape record [0026](0026-init-and-destroy.md) uses for `init`,
`destroy` and `copy` and record [0031](0031-what-copying-a-value-means.md) for
`to_string`. Hadley chose the real feature.

## Decision

**1. The syntax is `operator` and then the operator, written against it.**

```haard
def operator[] : T&
    @i : i32
    return data[i]
```

Adjacent, and reported when not: Haard is space sensitive on purpose, and
`operator []` would pretty-print as `operator[]` and stop round-tripping —
which the parser suite checks.

**2. The name is `operator[]`.** Not a keyword, not a flag on a declaration: a
**name**, interned like any other, and one that cannot be written as an
identifier. So no source can call one by name, and **nothing after the parser
knows an operator was involved**. Overload resolution, the override checker and
the emitter read an ordinary method. It is record
[0002](0002-generics-instantiated-in-the-importer.md)'s trick — the clone
`Pair#3#7` is declared under a name no source can write — used a second time.

One table keyed by the Ast node kind, `operator_name` in `ast/ast_node.cpp`,
because the parser and the typer have to agree on the spelling exactly: one of
them writing `operator+` and the other looking for `operator +` would simply
never find anything.

**3. The C++ does not use C++'s operator overloading.** Hadley's rule, and the
same argument record 0025 makes against C++ templates: the target language
having the feature is not a reason to hand the feature to it.

```cpp
h0_3_xs.m_operator_at_b6(3)                 // xs[3]
h0_3_xs.m_operator_at_b6(3) = 100           // xs[3] = 100
h0_3_xs.m_operator_eq_eq_rn1c23(h0_7_ys)    // xs == ys
```

The second line is a plain C++ assignment through the reference the method
gives back, which is not operator overloading — it is what a `T&` is.

**The mangling had to grow a table**, and it is load-bearing rather than
cosmetic. The emitter folds every character that is not alphanumeric to `_`, so
`operator+` and `operator-` would be one C++ name, and two operators of one
class taking one parameter of one type would collide. Each punctuation
character now gets a word: `_at`, `_plus`, `_minus`, `_times`, `_over`, `_mod`,
`_eq`, `_not`, `_lt`, `_gt`.

**4. A class has no builtin operators.** An operator a class did not overload
is reported, by name: `Point has no 'operator!='`.

This closes a hole that was already there and that nothing had found.
`ExpressionTyper::binary` asked whether the two sides were the same type, two
values of one class **are**, and the answer came back `bool` — so `a != b`
between two Points passed the type phase in **silence** and came out as a C++
`!=` that does not exist. Caught by g++ and not by `hdc`, which is the wrong
place for it. Found by an `Array<Array<i32>>` comparing its elements.

## Consequences

- **`Array<T>` is readable**, which is what this was for. `operator[]` gives
  back a `T&`, so reading one and writing through it are the same method —
  `xs[0]` and `xs[0] = 9`. That needs record 0018's amendment, which is
  [0035](0035-a-reference-is-the-thing-it-names.md).
- **An operator cannot call another operator**, because neither has a name to
  call. `Array` and `String` both put an ordinary `equals` underneath and have
  `operator==` and `operator!=` call it.
- **The typer asks only when the left side is a class.** A builtin `+` costs
  one comparison and nothing else changed shape; the emitter's cost is one read
  of the ResolutionTable per operator node.
- **Which declaration an operator node means is written on the operator node
  itself**, and not on a name inside it, because there is no name inside it.
  That is record [0019](0019-the-resolution-table.md)'s table used the way a
  call already uses it.

## What is not in it

**Unary operators** (`-x`, `not x`), **the compound assignments** (`+=`), and
**`=` itself**. None was needed and each is a line in one table plus a hook
where that operator is typed. `=` is the one that is not obviously additive:
record 0031 already gives a class a copy assignment, and an overloadable `=`
would have to say which wins.

## What it does not change

Nothing else in the compiler learned a word. The declaration is an
`AST_FUNCTION` with a `AST_BINDING_NAME` holding an `AST_IDENTIFIER`, exactly
as a written name is, and the difference is only its text.
