# TypeTable tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project. The golden is **the type the collector
gave every declaration**, rendered back as source, and the number of types the
module interned. The dumper builds nothing of its own: it reads the table the
compilation filled, so what a golden shows is the phase and not the test's idea
of it.

```
<unnamed>:
    Shapes.matrix  i32[10][5]
    Shapes.pointer_to_matrix  i32[10][5]*
    Shapes.five_pointers  i32[10]*[5]
  19 types interned
```

Rendering back to the written syntax is the check that matters. `i32[10]*[5]`
only comes out again if the nesting the parser built was walked in the right
order, and record 0016 says the table adds no nesting of its own. Those three
lines are Hadley's own examples of 2026-09-02: arrays are written like C++'s and
read unlike them, every postfix applying in the order it was written.

## The count line is not decoration

`a_type_is_written_once` interns 17 types: the sentinel, the thirteen builtins,
and then `i32[10]`, `i32[5]` and `{i32: i32}` — one each, though two of them are
written twice. The count is how a golden can say *nothing extra was interned*,
which is a thing no rendered line can say.

**It caught a real bug.** The first version of the poison rule was a helper,
`wrap(inner, table->pointer(inner))`, which evaluates the interning **before**
deciding to discard it. The rendered lines were right — `Missing*` came out as
`<none>` — and the count said 15 where it should have said 14, because a
pointer-to-nothing had been written into the table anyway. The check now comes
before the call.

## A function holds its whole signature

`a_function_holds_its_signature` is where record 0012 becomes visible:

```
    nothing  void
    one  i32 -> i32
    many  i32 -> T -> f64[10] -> Node*
    overloaded  i32 -> void
    overloaded  f64 -> void
```

Two candidates of one name, told apart by their parameter types and by nothing
else — which is what "a name maps to a list" was always going to mean once
there were types to distinguish them by. The return is the **last** argument,
per record 0016, so resolution reads every argument but the last and ignores it;
putting it first fails a case.

`many` is also why a function's signature is built in the scope the function
**opened** and not the one it sits in: `T` is the function's own generic
parameter, and it is only in view there.

## An operand of `and` is a bool or it is nothing

`a_logical_operator_is_bool` is record 0018 with no conversion meeting the one
place every other language quietly converts. There is no truthiness here: `n
and flag` with an `i32` `n` is not a mistake about a type it could be turned
into, it is simply not a bool, and the binding it was given to has **no type at
all** — the golden shows `<none>` next to it.

Both spellings are one rule (`and` and `&&`, `not` and `!`), which the parser
keeps apart only so the printer can write back whichever was written.

## What the goldens pin

- **Every shape the grammar has**: pointer, `**` as two pointers, `***`,
  reference, list, hash, tuple, sized and unsized array.
- **The array length is part of the identity.** `i32[10]` and `i32[5]` are two
  types, and dropping the length from the entry collapses them — 3 of 5 cases
  fail.
- **A named type and a generic parameter.** `Pair<i32, Pair<f32, bool>>`
  nests, and inside `Pair<A, B>` the fields are the generic parameters
  themselves, resolved in the scope the declaration opened.
- **A part that cannot be built poisons what contains it.** In
  `a_type_that_names_nothing`, `Missing*` is no type at all rather than a
  pointer to nothing, and a `def` with a `Missing` parameter or a `Missing`
  return has no signature rather than a wrong one. The missing name is the
  `UseResolver`'s diagnostic and not this pass's, which is why the golden has
  no error in it.
- **A declaration that defines a type is one.** A class candidate holds the
  named type pointing back at itself. Its generic arguments are not part of it:
  `Node` is the declaration, `Node<i32>` is a use of it, built where written.
- **A `let` with no written type stays untyped.** Inference does not exist yet,
  and the golden says so by showing nothing rather than guessing.

## Inference, and the literal that has no type

Record 0018 makes a literal take the type its context asks for, and gives it a
default when nothing does. `a_binding_takes_its_type_from_what_it_was_given`
is the whole rule in ten lines: `a` is an `i32` and `b` an `f64` by default,
`n : i64 = 1` makes the literal an `i64` **without converting anything**, and
`wide : u64 = 4294967296` holds a value no `i32` could.

`a_literal_that_does_not_fit` is the diagnostic shape the record asked for —
about the **value**, not about a type:

```
error: 300 does not fit in u8
error: expected i32, found a floating point literal
error: expected f64, found an integer literal
```

`nothing_converts_implicitly` is rule 2 with no exceptions: `i64 = a` where `a`
is an `i32` is an error, and `a + n` across two widths is an error at the
operator, because there is nothing in the language that would make them one
type.

**A value does not bind to a reference parameter, and that is not decided.**
`upcast(r)` with an `r : Derived&` matches a `Base&` parameter;
`upcast(b)` with a `b : Base` **value** does not, and
`a_call_that_matches_nothing` holds the error. Record 0018's coercion list says
"a reference or a pointer to a derived type where a base is expected" and says
nothing about a plain `T` reaching a `T&` — which is the ordinary way a
reference parameter is called in C++. The strict reading is in the golden so
that answering it changes a file; it may well need a fourth entry on that list.

**One sub-rule is stricter than Go's and Zig's, deliberately.** `let x : f64 = 1`
is rejected here; both of those languages accept an integer literal in a float
context when the value is exactly representable. Refusing is the tighter
reading of record 0018's rule 1, and record 0018's own argument applies:
allowing it later breaks nothing, and forbidding it later breaks everything.
The golden holds the error so that changing one's mind changes a file.

## Members, inheritance, and the dot

`a_member_is_found_through_the_type` is where the whole family joins up. The
right side of a `.` is not a name in any scope, so nothing before this could
answer it: the left side's type is a `TYPE_NAMED`, which points at a class's
**candidate**, whose **scope** holds the members, and an inherited one is the
same search one step up `Candidate::super`.

```
    by_value  f64            d.value
    inherited  i32           d.tag,  declared on the base
    sugar  f64               p.value where p is a Derived*
    sugar_inherited  i32     both at once
    explicit  f64            p->value
```

**`.` reads a member of a `T` and of a `T*` alike** — Hadley, 2026-09-02, and a
deliberate difference from C++. `->` is the explicit form and only a pointer may
be written with it. Both look through exactly **one** level, so a `Derived**` has
members under neither, which `a_member_that_is_not_there` records. That rule is
on record 0018's closed coercion list, because it is an implicit step the
compiler takes on the reader's behalf and that list is what such steps go on.

**Single inheritance** is what keeps `Candidate::super` one field and the climb
a walk rather than a search. Hadley settled it the same day; interfaces are
agenda 1.16 and would make it a second axis.

## A bug this step exposed in shipped code

The `UseResolver` refuses the right side of an `AST_DOT` on purpose — it is a
member and not a name, and resolving it through the scope chain would find a
global of that name in silence. It was **not** refusing the right side of an
`AST_ARROW`, so `p->value` was being resolved as a bare `value` and would have
picked up any global called that. Nothing failed, because no case had written
`->` until now. Two cases fail with it put back.

## Choosing an overload at a call

`a_call_chooses_by_signature` is the second half of agenda 2.7 running.
`take(1)` picks the `i32` one and `take(2.5)` the `f64` one, which is record
0012's overload distinction being *used* rather than merely stored, and record
0018 is what makes it short: an argument matches a parameter when the two types
are **equal**, when it is a **literal that can be that parameter**, or when it
is a reference or pointer to something **derived** from it. There is no
conversion to rank.

**A literal is carried untyped into the call.** Typing `3` before choosing would
make it an `i32` and then `small(200)` would fail against a `u8` parameter. Each
candidate asks the literal to be its own parameter instead, which is rule 1
meeting rule 5.

**Arity is a range**, because a default value makes it one (record 0012).
`with_default(1)` and `with_default(1, 2)` both answer, `with_default()` does
not, and `a_call_that_matches_two` is the case record 0012 predicted: two
declarations that both answer to one count, which is not wrong until a call is
written and then is an ambiguous **call**.

## A method call, and `this`

`a_method_is_called_on_its_object` closes the expression typer's last gap.
A method call and a field access are **the same walk** — the class's scope and
then up `Candidate::super` — and they differ only in what they take off it: the
field access reads the first answer's type, the call takes the whole set and
hands it to the overload resolver.

```
    no_args  f64        d.compute()
    one_arg  f64        d.compute(1.5),  the other overload
    sugar  f64          p.compute() where p is a Derived*
    inherited  i32      d.describe(),  declared on the base
    arrow  i32          p->describe()
    mine  i32           this.describe(), inside a method
    myself  Derived*    'this' is a pointer
```

**A base's method is an overload of the derived class's, not something it
hides** — record 0012's scope steps contributing to one candidate set, with a
base as one of those steps.

**`this` is a `Class*`**, which is what a method receives, and record 0018's
`.` reads a member of a pointer and of a value alike so it is written the same
either way. `myself : Derived* = this` is what makes that testable at all:
through a `.` the two are indistinguishable, and **both sabotages against
`this` passed until that line existed**.

**An unknown method is reported here and nowhere else.** The `UseResolver`
skips the right side of a dot on purpose, so `t.missing()` has no other phase
that could complain about it.

## Types crossing a module boundary

`a_call_across_a_module` is the first thing to exercise record 0016's
translation. `make` returns a `Point*` declared in `lib.shapes`, and `p` in
`app.main` holds that type **in app.main's own table** — the count for that
module goes up by two because the pointer and the named type were interned
there. Skipping the translation of either a parameter or the result fails a
case.

It also caught a phase-order bug that no single-module case could: **inference
cannot run inside the walk that fills the tables.** `let p = make(1)` resolves a
call whose candidate lives in a module the walk had not reached yet, so the type
phase is two passes over every module — what each declaration *wrote*, then what
each binding was *given*. Folding them back into one fails this case and only
this case.

## The sabotages

| sabotage | fails |
|---|---|
| a builtin's index is not fixed, but seeded lazily | 5 |
| no interning: every type is appended | 3 |
| the array length is left out of the identity | 3 |
| the postfix nesting is applied outermost first | 3 |
| a poisoned part is interned anyway | 1 |
| the type collector never runs | 6 |
| a type declaration gets no type of its own | 6 |
| a function's signature is built in the scope it sits in | 1 |
| a function's parameters are left out of its signature | 1 |
| the return type comes first instead of last | 1 |
| a poisoned parameter does not poison the signature | 1 |
| the expected type is not handed down to the literal | 3 |
| a literal is never checked against its width | 1 |
| an integer literal may be a float | 1 |
| the two sides of an operator need not match | 1 |
| a comparison has the type of its operands | 1 |
| nothing is inferred: a binding keeps only what it wrote | 1 |
| the written type is not checked against what was given | 1 |
| the base is never recorded | 1 |
| the member walk never climbs to the base | 1 |
| `.` does not look through a pointer | 2 |
| `->` is accepted on a value | 1 |
| `.` looks through every pointer instead of one | 1 |
| the arrow's right side goes through the scope chain again | 2 |
| a literal is not asked to fit the parameter | 2 |
| arity is a number, not a range | 1 |
| a tie is not ambiguous, the first wins | 1 |
| the return type is not translated into the caller | 1 |
| a parameter is not translated into the caller | 1 |
| an upcast climbs no chain | 1 |
| inference runs inside the first pass | 1 |
| a method callee is not recognised | 2 |
| the member walk never climbs to the base | 2 |
| only the first candidate of a member name is gathered | 1 |
| `this` has no type | 1 |
| `this` is the class and not a pointer to it | 1 |
| `same` compares only the kind and the argument count | **0** |

The last one is the confirm that follows a hash hit, and **nothing here reaches
it**: the mixing hash separates every type in these cases, so the comparison is
never the thing that decides. It is the same position the `StringTable`'s byte
comparison would be in without a real collision to hand — and unlike FNV-1a's
published collisions, there is no way to write two colliding types by hand,
because the hash mixes fields rather than bytes of text.

Read it as intent, not tested behaviour. If it is ever wanted as tested
behaviour, the way there is to key the deduplication map on the fields
themselves and delete `same` entirely, which trades memory for one fewer
untestable branch.
