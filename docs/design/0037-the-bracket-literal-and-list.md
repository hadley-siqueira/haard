# 0037 — What a bracket literal builds, and how `List<T>` is reached

Status: **decided and implemented**, 2026-09-06. **`char*` → `String` moved
onto this mechanism on 2026-09-08** — see the last section. Hadley settled it over three
rounds; the design note that framed the choices is
[notes/array-list-and-the-bracket-literal.md](notes/array-list-and-the-bracket-literal.md).

| | |
|---|---|
| `[1, 2, 3]` **infers** as an `Array<T>`, always | **decided** |
| `{1, 2, 3}` is a **fixed** array and the primitive everything stands on | **decided**, record 0021 unchanged |
| A literal is a fixed array plus **one** constructor call, not a run of `add` | **decided** |
| The fixed array is a file-scope `static` when every element is a literal | **decided** |
| A **written** class type takes a literal by a constructor the compiler picks | **decided**, and only a literal |
| It reaches a call, a return, a binding and an assignment | **decided** |
| `List<T>` is a doubly linked list, reached only by writing its name | **decided** |
| `[T]` is written form for `List<T>` | **unchanged**, record 0022 |

## The shape it all collapses to

Hadley's `List<i32>(i32*, i32)` is what made this small. There is no literal
for a `List`, so everything is `{...}` plus a `(T*, i32)` constructor:

| written | means |
|---|---|
| `{1, 2, 3}` | an `i32[3]`, a C++ array and nothing else |
| `[1, 2, 3]` | `Array<i32>(<that>, 3)` |
| `let b : List<i32> = [1, 2, 3]` | `List<i32>(<the Array>)` |
| `let b : List<i32> = {1, 2, 3}` | `List<i32>(<that>, 3)` |

One `add` per element became **one constructor call**, and the fixed array
literal — which had never been emittable — turned out to be the thing
everything else stands on.

```cpp
static int32_t __fx0[3] = {1, 2, 3};
h1_Array __ar1(__fx0, 3);
h2_List h0_3_a(__ar1);            // let a : List<i32> = [1, 2, 3]
h2_List h0_4_b(__fx2, 3);         // let b : List<i32> = {1, 2, 3}
```

**Nothing there is a C++ implicit conversion.** `let hello : String = "hello"`
works today only because record 0026 emits `init` as a C++ constructor and
C++'s own conversion does the rest — the shape Hadley ruled out for operators
in record 0034. This is that rule applied to constructors: Haard picks, and the
emitter writes a call it can see.

## Where the fixed array goes

A **file-scope `static`** when every element is a literal, a **local** when
not. "Constant" is `every element is a literal token` — syntactic, and it needs
none of agenda 5.3's evaluation.

Not `const`, because record 0029 leaves Haard without one and the constructor's
parameter is a plain `T*`. The class copies out of it, so nothing writes
through it.

The statics are spliced **above the globals** and not merely above the bodies.
`let shared = [1, 2, 3]` at module level is a global, and with the splice one
line higher its array came out after the line that named it — caught by g++ and
not by `hdc`, which is the wrong place.

## What is refused

**Hoisting.** A literal that is not already bound to a name becomes one, and
that is a statement — so record 0032's three places come with it: a loop
condition, the right of `and` or `or`, and module level with no statement to
be built before. A **global** is not one of those: a global is a binding, and
the emitter builds one where it is bound.

Hadley chose a **named local** for the intermediate over a `const_cast` or
going straight to `(T*, i32)`. It costs this: a literal used where a
**reference** is expected — which is most of them, since `Array<T>&` is how one
is passed — needs a name to bind to. So the static placement buys less than it
looked like it would, and what it does buy is that the array underneath is
written once for the whole program.

**Anything that is not a literal.** `let l : List<i32> = some_array` is
refused. The alternative was C++'s converting constructor, which fires silently
for every class in the program and has `explicit` to defend itself while Haard
would have nothing. That door does not close again once programs depend on it,
and it deserves its own record with *does Haard need `explicit`* asked in it.

## Two holes it needed closed

### `T[N]` did not decay to `T*`

`takes(fixed, 3)` against `takes(i32*, i32)` was *no 'takes' takes these
arguments*. On record 0018's list now, free and only to the element's own type
— a decay is an address and not a conversion.

### The body of an **unbound generic** was checked

`List<T>` holds a `Node<T>*`, and `walk = walk->next` was *cannot assign
Node<T>* to Node<T>**. Two occurrences of `Node<T>` written in two generics are
two types: each `T` is a parameter of its own declaration and interns on its
own, so `head` was built with List's and `walk->next` with Node's.

Record 0002 already said a use of a generic names the **clone**. So the
**StatementChecker** skips a declaration whose parameters nothing has bound,
and the clone — where every parameter has a type — is what gets checked.

**Only the statement checker.** Collecting the types of a generic's members is
harmless and still happens, which is why the goldens that show `T` and `i32`
inside an uninstantiated `Pair<T>` are unchanged. Making the type collector
skip them too was tried, changed three goldens, and turned out to fix nothing.

## Where the constructor is resolved, and why it is in two places

A **written** class type resolves its constructor in the typer, where it
belongs: `List<i32>` was instantiated by the written type, during collection,
so its signatures exist by the time a literal is typed.

`Array<i32>` for a bare `[1, 2, 3]` is not. `build_generic` clones it *during
inference*, and a clone's candidates get their signatures from a later sweep —
asked immediately, the candidate count comes back zero and the message blames a
class that is fine. So **the emitter** asks whether `Array<T>` has its
`init(T*, i32)`. That is one place further down than a diagnostic wants to be,
and it is a **library** invariant rather than something a program can get
wrong.

## `List<T>`

`std/list.hd`, written in Haard. A `Node<T>` with `value`, `next` and
`previous`; a `List<T>` with `head`, `tail` and `size`. It owns its nodes, so
record 0031 makes it write a `copy`, and that `copy` takes a `List<T>&` —
a generic naming itself, which record 0036 had to make emittable first.

Three constructors and they are the whole of how one is built: empty, from an
`Array<T>&`, and from a `T*` and a count.

**`operator[]` yes** — Hadley, 2026-09-06. It walks from the head, so it is
O(n) and a loop over `l[i]` is O(n²); that is the author's to know and not the
language's to forbid. It gives back a `T&`, so `l[2] = 9` works.

## Still open

- **Whether record 0018's list composes.** `char*` → `String&` needs two
  entries and gets none. Record 0035 left it open and this record did not need
  it, because a written class type is answered by a constructor and not by the
  list.
- ~~**An empty literal.**~~ **Decided and built 2026-09-06.** Hadley: it takes
  its type from the **context** where the context says, and is an error where
  it does not — *infer where it is trivial, do not try hard*. The context is a
  written type, on the binding it is given to or the variable it is assigned
  to; a **call** is deliberately not one, because an argument is typed before
  the overload is chosen.

  It is also the one literal the sugar pass **leaves where it stands**, and not
  as an exception: what a literal is hoisted for is the fixed array it is made
  of, and an empty one is made of nothing — while moving it would throw away
  the only thing that can say what it is empty of. It emits as
  `Array<T>(nullptr, 0)`, one call pointing at nothing, since the
  constructor's loop runs zero times.

  Reading `T` back out of an `Array<T>` needed the **instantiation** and not
  the type: record 0002 gives a clone no arguments in its type, so
  `Array<f64>` is a plain named type whose fields are already `f64`.
- ~~**Migrating `char*` → `String` onto this mechanism.**~~ **Done
  2026-09-08**, in the half that needed no new syntax. See below.


## `char*` → `String` on this mechanism, 2026-09-08

Hadley asked for it. It landed in the half that needs nothing new, and the
half that is left is named at the end.

### What moved

**A written string literal reaching a class type is a construction this
compiler chooses.** `ExpressionTyper` asks `constructed_by_one` — the
one-parameter shape of `constructed_from`, split out because a string literal
has no count to write and would otherwise match an `init(char*, i32)` with
nothing to pass as the second argument — and writes the chosen `init` **on the
literal**. From then on the literal *is* that class.

The emitter reads that and writes the call, at all four places a value is
given to something. Only one of the four changed its C++, and that one is the
whole point:

```cpp
h0_1_String h0_25_gives() {
    return "returned";                  // before: C++ found the conversion
    return h0_1_String("returned");     // after:  the compiler wrote the call
}
```

The other three already wrote a construction — `String s("abc")`,
`takes(String("abc"))`, and the `const_cast` dance for a reference — but C++
was choosing *which* constructor. Now the choice is in the ResolutionTable,
where `tests/resolution_table/cases/a_literal_says_which_constructor_it_meant`
prints it.

### The name stopped mattering, for a literal

Record 0023 made this a **named** relation: the compiler knows the class
`String`. That is still true of a `char*` **value** — `let s : String = p` is
record 0018's list and nothing else, and the list knows the one name.

A written **literal** reaches *any* class that says how to build one of itself
out of a `char*`, which is this record's rule and not 0023's. `Note` in the
emitter and resolution-table cases is a class the compiler has never heard of,
and `let note : Note = "note"` builds one.

At a **call** the ranking had to learn it too, since a call ranks before
anything is handed down: `OverloadResolver::match` costs a written literal into
a constructible class **one step**, which is exactly what record 0023's entry
cost, with the name taken out. So `write("abc")` between `write(char*)` and
`write(String)` still takes the first — `char* first` still falls out of the
list rather than being written into the ranking.

### The bug it found on the way

```haard
let s : String
s = "abc"
```

Haard accepted it (the list says a `char*` fits a `String`) and the emitter
wrote `s.m_assign("abc")` — record 0031's copy assignment takes a `String&`,
so **g++ refused it**: *cannot bind non-const lvalue reference to an rvalue*.
It compiled here and failed there, which is the failure this record exists to
prevent, and it had been shipped since record 0031.

The fix is one call: `emit_copy_assignment` runs the right side through
`emit_conversion`, the same helper an argument goes through, which both builds
the class and gives the temporary something to bind to.

It had never fired because the two classes in the suites that are assigned a
literal both declare `operator=` taking a `char*` (record 0034), which is a
different path — and because no case had assigned a literal to a class that
does not.

### What is left, and what it waits on

Deleting record 0023's named entry, so that a `char*` **value** reaches no
class at all. Then `let s : String = p` needs `String(p)` written by hand, and
there is no syntax for constructing a value with arguments — the same gap
record 0040's `iterator()` ran into. That is the last step and it is a
question about `T(args)`, not about this record.

The **bracketed** literals still do not reach a call, a return or an
assignment, for a reason that is this record's own doing: they are **hoisted**
into a binding before any of those is reached, so what arrives is a name and
not a literal. A string literal is not hoisted, which is why it could move
today. Whether the hoist should learn about the type it is being given to is
open.
