# 0073 — A compound operator on a class

Status: **decided and built**, 2026-10-07. Hadley accepted the recommendation
of the second probe round: `a += b` on a class is `a = a + b`, as in Python.

| | |
|---|---|
| `a op= b` on a class, for `+ - * / // %`, is `a = a op b`, through the class's own `operator op` and its assignment | **decided**, Hadley |
| A target that is not a name is read **once**, through a reference | **decided while building it** |
| `&= \|= ^= <<= >>= >>>=` on a class are refused: a class declares no bitwise operator | **decided while building it** |

## Before

A class cannot declare `operator+=` -- record
[0034](0034-an-operator-is-a-method-with-an-unwritable-name.md)'s table has no compound names -- and
the statement checker let `a += V(2)` through; the emitter wrote a C++ `+=`
nobody had defined, and g++ refused it.

## How

`StatementChecker::check_assignment` rewrites the statement in place, and
then checks what it became like any assignment:

```haard
a += V(2)              # a = a + V(2)
h.items[next()] -= v   # let __tc0 : V& = h.items[next()]
                       # __tc0 = __tc0 - v      -- 'next' runs once
```

The right side is the operator's argument, so `a *= 4` asks each `operator*`
to take the 4 (record 0018). A reference on the left writes through (record
[0035](0035-a-reference-is-the-thing-it-names.md)).

Cases: `tests/emitter/cases/a_compound_operator_on_a_class` (15) and
`tests/statement_checker/cases/a_class_has_no_bitwise_compound`. Four
sabotages, each caught.
