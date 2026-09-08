# 0036 — `Array<T>`, and the three holes writing it found

Status: **decided and implemented**, 2026-09-06. It decides nothing new about
what `T[]` *means* — records [0016](0016-the-type-table.md) and
[0022](0022-the-standard-library-and-what-is-sugar.md) did that — and records
how it was finally built, plus three holes that only a generic container could
have found.

| | |
|---|---|
| `T[]` is rewritten into `Array<T>` by record 0025's Ast → Ast pass | **decided** |
| `T[3]` is untouched: a written length is a **fixed** array (record 0021) | **unchanged** |
| A generic naming **itself** is not an instantiation | **decided**, and it was a blocker |
| A class with `copy` and no `init` gets a defaulted C++ default constructor | **decided**, a bug |
| A generic's body is checked with its parameters **unbound** | **open**, and named here |

## Why it could not be built before

`T[]` has been written form for `Array<T>` since record 0016, and two things
were missing. **Generics**, which landed 2026-09-05 (record 0002 made real).
And a way for the compiler to put the name `Array` in view of a file that never
wrote an import — which is record [0033](0033-the-prelude-is-a-list-of-automatic-imports.md)'s
prelude, 2026-09-06. Record [0017](0017-the-prelude.md) was written for exactly
this and said so.

## Decision: the sugar is a rewrite, in the pass that already exists

`SugarLowerer` turns an `AST_ARRAY_TYPE` with **no length** into
`AST_NAMED_TYPE` naming `Array` with one generic argument. Nothing downstream
has a case for an array with no length: `TypeBuilder` builds an ordinary
instantiation, record 0002 clones it, the emitter writes a struct.

It hoists nothing — a type is not an expression — so unlike record
[0032](0032-how-a-template-string-is-lowered.md)'s template strings it can be
written anywhere a type can, including a field and a parameter.

`i32[][]` works and is `Array<Array<i32>>`: the pass walks the children first.

**The shape is not gone from the TypeTable.** `new T[n]` with a length no
constant evaluation can read still builds a `TYPE_ARRAY` with `NO_LENGTH`
(record [0028](0028-a-heap-array-is-a-pointer.md)); it just cannot be *written*
as a type any more, so the emitter's *"an array with no length cannot be
emitted yet"* is no longer reachable through the sugar.

## The three holes

### 1. A generic that names itself could not be emitted

Record 0031 makes every container that owns memory write

```haard
def copy : void
    @other : Array<T>&
```

and `Array<T>` there has a `T` **nothing has bound**. It was instantiated: a
clone whose fields are type parameters, a real declaration, emitted like any
other and refused with *"this type cannot be emitted yet"*. `Array` could not
be written at all.

**It is not an instantiation.** It is the declaration being written down, and
it is only ever read from inside the generic's own body — which the emitter
never writes on its own. So `TypeBuilder::build_named` gives back the generic
declaration as a named type when **any** argument is a `TYPE_GENERIC`.

Inside the clone the same node re-resolves with `T` bound, because record 0002
binds a parameter by giving its candidate a type — so `Array<T>` inside
`Array#i32` builds `Array<i32>` and finds the clone itself. C++'s injected
class name, arrived at from the other direction and for a different reason.

### 2. A class with `copy` and no `init` could not be built, in C++ only

Declaring a constructor is what takes C++'s implicit default one away, and
record [0026](0026-init-and-destroy.md) leans on that one: a class with no
`init` is built with no arguments and its fields are left uninitialised. So the
moment record 0031 emitted a copy constructor, such a class lost the ability to
be built at all — `let p : Pair<i32>` compiled in Haard and failed in **C++**,
which is the worst place for it.

A defaulted default constructor is emitted beside the copy one. Found by
`tests/emitter/cases/a_generic_names_itself`.

### 3. Two class values compared with a builtin operator

Covered by record [0034](0034-an-operator-is-a-method-with-an-unwritable-name.md),
and it is listed here because this is what found it: an `Array<Array<i32>>`
comparing its elements with `!=` when only `==` was overloaded.

## What `Array<T>` is

`std/array.hd`, written in Haard, in the test cases that need it. It owns a
`T*`, doubles when full, and writes record 0031's `copy` — so an
`Array<String>` copies each element with the element's own `copy` and no two
Arrays hold one buffer. Record 0034's `operator[]` gives back a `T&`, and
`operator==`/`operator!=` sit over an ordinary `equals` because neither
operator can call the other.

## Open, and named rather than fixed

**A generic's body is type-checked with its parameters unbound.** So

```haard
class Pair<T>:
    left : T

    def init : void
        left = 0        # error: a literal cannot be T
```

is reported, although every instantiation of it would be fine. Record 0002 says
the clone is what a use means, and the uninstantiated declaration is checked
anyway. Nothing has decided whether it should be, or whether a parameter should
absorb a literal the way record 0018 lets a written type do.

**`[1, 2, 3]` is still a `TYPE_LIST`** and is not yet an `Array<i32>`. Hadley
decided 2026-09-06 that a bracket literal **infers** as an `Array<T>` and is a
`List<T>` only when explicitly typed, with `List<T>` taking an `Array<T>` in
its constructor. Neither the literal nor `List` is built.
