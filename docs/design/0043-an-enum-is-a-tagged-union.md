# 0043 — An enum is a tagged union, and `switch` is how it is read

Status: **decided and being built**, 2026-09-08. Hadley gave the semantics and
every piece of syntax; this records what was decided, what it compiles to, and
what is not written yet.

> Enums in Haard are sum types, tagged unions, algebraic data types as far as
> the semantics go.

| | |
|---|---|
| An enum is a **sum type**, and the payload-free one is its degenerate case | **decided**, Hadley |
| A variant with a payload is a **constructor**; a tuple payload is flattened | **decided while writing it** |
| It is built `Action.Click(10, 20)` **or bare** `Click(10, 20)`, and bare has **low priority** | **decided**, Hadley |
| It is read by `switch`, whose `case` is a **pattern** that captures | **decided**, Hadley |
| **No fall through and no `break`**; cases group by writing one with no block | **decided**, Hadley |
| Grouping cases that bind **different names** is an error | **decided**, Hadley |
| A `switch` also walks an integer, a char, and later a String or a tuple | **decided**, Hadley |
| Recursion is written with a **pointer**; by value is an error | **decided**, Hadley |
| A payload that owns memory gets a **destructor and a copy that ask the tag** | **decided**, Hadley |
| A capture is a **reference** into the value being switched over | **decided while writing it** |
| A `switch` over an enum is **exhaustive** or writes `default` | **decided while writing it** |

## The syntax, which is Hadley's

```haard
enum Action:
    Idle
    Click : (i32, i32)
    Code = 7
    Move : (i32, i32) = (0, 0)
```

`name ':' type ('=' expression)?`, and the parser has read it since
2026-08-23. A written value on a variant that carries **nothing** is its tag;
on one that carries something it is a **default payload**, which is not
written yet and is refused by name.

```haard
let a = Action.Click(10, 20)
let b = Click(10, 20)          # bare, and low priority
```

*"Anything in the current scope called Click wins."* So the bare
form is looked for **last**, after the scope chain and after the imports, in
`NameResolver::resolve` — which means no program that resolved before resolves
differently now, and `Action.Click` is what to write when something else wins.

```haard
switch a:
    case Click(x, y):
        return x + y

    case Idle:
        print("idle...")

    default:
        print("on default case")
```

**No fall through and no `break`.** Cases are grouped by writing one with **no
block**: it runs the block of the one below it.

```haard
switch a:
    case Up:
    case Down:
    case Increment(x):      # an error: this group does not bind one set of names
        print("Ok!")
```

Hadley's rule: grouping cases that bind **different names** is an error, and
the same example without the `x` is fine. Binding the same name for two
different types is the same mistake one step in, and is reported too.

## What it compiles to

**Nothing carries anything** — a C++ `enum class` over an `int32_t`. Scoped, so
a variant's name cannot collide, and with no implicit conversion to an integer,
which is record 0018's rule arriving in the emitted code for free.

**Something carries something** — a struct with a tag and an anonymous union,
and a **maker per variant**:

```cpp
struct Action {
    int32_t tag;
    union {
        struct { int32_t _0; int32_t _1; } Click;
    };
};

Action Action_Click(int32_t a0, int32_t a1) {
    Action made;
    made.tag = 1;
    made.Click._0 = a0;
    made.Click._1 = a1;
    return made;
}
```

A maker and not a designated initialiser, because that is C++20 and this emits
C++17 — and because a call to one is what `Action.Click(10, 20)` already is, so
nothing on the expression side had to learn a new shape.

The payload is **flattened**: a tuple of two is two fields and two arguments,
which is what the constructor's signature says and what Rust gives a tuple
variant. It also means a tuple never has to be emitted, which is just as well.

The tag is the **C rule**: a counter that an explicitly written value resets.
One function decides it, because the maker that sets it and the case label that
tests it have to agree.

### When a payload is a class

A union whose members are classes has **no** default constructor, destructor or
copy of its own: C++ deletes all three, because it cannot know which member is
alive. The tag knows, so the struct writes them, plus the two the tag makes
possible:

```cpp
    Action() { tag = 0; }
    Action(const Action& other) { __copy(other); }
    ~Action() { __clear(); }
    void m_assign(Action& other) { if (this != &other) { __clear(); __copy(other); } }

    void __clear() {
        switch (tag) {
        case 1: Say._0.~Text(); break;
        default: break;
        }
    }
    void __copy(const Action& other) {
        tag = other.tag;
        switch (tag) {
        case 1: new (&Say._0) Text(const_cast<Text&>(other.Say._0)); break;
        default: break;
        }
    }
```

`m_assign` and not a C++ `operator=`, which is record 0034's rule; destroy
before copy, which is record 0031's. Placement new needs `<new>`, which is the
third header this emitter includes.

None of it is written when no variant carries a class: the union is then
trivially copyable and C++'s own answers are the right ones.

## What a `switch` compiles to

A C++ `switch`, over the value for an enum and an integer and over `.tag` for a
tagged union, with a `break` this emitter writes at the end of every block and
two labels in a row where Haard grouped two patterns. Every body is braced,
because a `case` that declares a local is illegal in C++ without them.

A switch that **captures** binds its subject to a name first — a reference when
the subject has one to refer to, a copy when it does not — so an expression
with a side effect is read once and not once per name:

```cpp
{
    Message& __sw0 = m;
    switch (__sw0.tag) {
        case 1: {
            String& text = __sw0.Say._0;
            ...
```

**A capture is a reference**, which is record 0040's loop variable one
construct over and for the same two reasons: a copy of a payload that owns
something is an allocation per match, and a payload that cannot be copied could
not be taken apart at all. Writing through one writes into the value being
switched over.

For a group, every case binds the same names for the same types, so what
differs is only which member of the union the value comes out of — one
conditional on the tag per name.

## What is checked

- **exhaustiveness**: every variant, or a `default`. This is the feature people
  buy an enum for, and it is what turns adding a variant into a list of the
  places that have to change. Not asked of an integer or a char, where there is
  no covering every value
- a pattern that names **no variant** of the subject, by name
- the **same variant twice**, and two `default`s
- a pattern that takes apart **more or fewer** things than its variant carries
- a group that binds **different names**, or the same names for different types
- a case with **no block and nothing after it** to share one with
- a variant that carries **the enum it belongs to by value** — no size; write a
  pointer, and nothing allocates behind the author's back
- a variant that carries a class that **cannot be copied**, since the union's
  copy asks each variant to copy what it holds

## A switch that cannot be one

*"You may translate it to if/else if that is easier."* An enum, an integer and a
char stay a C++ switch, because a jump table is what that statement is for.
**Everything else becomes a chain**, in a pass of its own (`SwitchLowerer`)
that runs in the type phase after inference — it has to know what it is
walking, which is record 0040's reason one statement over:

```haard
switch name:                {
    case "ready":               let __sw0 = name
    case "done":                if __sw0 == "ready" or __sw0 == "done":
        f()                         f()
    default:                    else:
        g()                         g()
                            }
```

The subject is bound **once**, and to a reference when it has a name to refer
to, so a chain over a String copies nothing and `switch f():` calls f once. The
patterns of a group are joined by `or`, which is what sharing a block means
when there is no switch to fall through.

What resolves the comparison is the ordinary machinery: the statement checker
types an ordinary condition, so `operator==` is found the way record 0034 finds
it, and a subject whose class has none is reported in those words.

It also generalised record 0037 one more step: a written literal reaching a
**class reference** parameter — `operator==(@other : Name&)` — is a
construction now, which is what record 0023's second named entry did for
`String&` alone.

## The default payload

`Move : (i32, i32) = (0, 0)` is a variant whose payload has a default, and it
means the maker may be called with nothing:

```cpp
Action Action_Move(int32_t a0 = 0, int32_t a1 = 0) { ... }
```

carried into the C++ the way a parameter's default already is. Two consequences
fall out: `required_of` says such a variant requires **nothing**, and writing
its name alone — `let a = Move` — is a **value of the enum** rather than the
constructor, which is `ExpressionTyper::value_of_candidate`. The default is
written out, one value per thing carried; a name of a tuple is refused, since a
tuple is not something this back end can take apart.

## What is not written yet

- a variant used as a **value** without being called and **without a default**,
  `let f = Action.Click`, which is a constructor as a first-class function
- a **pattern that is not a variant or a written value**: no ranges, no guards,
  nothing nested. A pattern is a name, a value, or a variant taken apart

## In code

`AST_SWITCH`, `AST_CASE`, `AST_DEFAULT` and the three keywords; `SYMBOL_VARIANT`
in the symbol table; `TypeCollector::payload_of` and `capture_of`;
`NameResolver::gather_variants`; `StatementChecker::check_switch`; and in the
emitter `emit_enum`, `emit_tagged_union`, `emit_union_lifetime`, `emit_switch`
and `emit_captures`.

Cases: `tests/emitter/cases/an_enum_without_a_payload`,
`an_enum_that_carries_something`, `an_enum_that_carries_a_class` and
`a_switch_is_a_pattern_match`; `tests/type_table/cases/a_variant_is_one_of_its_enum`;
`tests/statement_checker/cases/a_switch_covers_its_enum` with fourteen
verdicts; and six in `tests/parser/`.

## Amended 2026-09-30: a generic enum

`enum Option<T>` parsed, and a clone of it worked when it was **written as a
binding's type** and given a bare variant: `let a : Option<i32> = Some(3)`.
Almost nothing else did, and the gaps table of 2026-09-24 had it as one line.
What was measured:

| written | before |
|---|---|
| `Option<i32>.Some(3)` | *'a' has no type the emitter can write* |
| `Option<i32>.None` | *'Option' names no declaration*, from the emitter |
| `Option.Some(3)` | *no 'Some' takes these arguments* |
| `let a : Option<i32> = None` beside an `Option<bool>` | *'None' names no declaration*, from the emitter |
| `let a = Some(true)` with no `Option<bool>` anywhere | *no 'Some' takes these arguments* |

Three things were wrong, and they are fixed.

**A clone made mid-inference had variants with no type.** Record 0039 types a
clone where it is made, and `TypeCollector::type_signature_now` typed its
methods and its fields -- and not its variants. `Option<i32>.Some(3)` makes the
clone and ranks the call against its `Some` in the same moment, so the answer
was *no 'Some' takes these arguments*: true, and about a signature nobody had
built. It is the shape of record 0039 once more: a clone made after a phase never
gets that phase. A variant is typed there now, the way a field is.

**`Option<i32>` where a value goes had no meaning.** The left of `Tree.Leaf` is
the name of a type and is typed as that type; a generic name there fell to the
typer's `default`. It is built the way a written type is, which is what makes
the clone, and the clone is written on the node for the emitter
(`ExpressionTyper::generic_enum`).

**A variant written without the enum's arguments named the generic's own
variant**, whose payload is a `T` and which is never a value. It is now the
same variant **of a clone**, and which clone is said by two things, in the
order they are trusted (`ExpressionTyper::variants`):

1. the enum the **context expects** -- `let a : Option<i64> = Some(3)` is an
   `Option<i64>`, where the literal alone would say i32; `Option.None` and bare
   `None` are found the same way;
2. then the **arguments**, solved against the payload by record 0059's rules,
   the way a generic function's parameters are: `Option.Some(true)` and bare
   `Some(2.5)` make and use `Option<bool>` and `Option<f64>`.

When neither says, it is an error naming what to write:

```
error: nothing here says what 'T' is, so the enum has to be written with it: Option<...>.None
error: nothing here says what 'B' is, so it has to be written: Either<...>.Left(...)
```

And the one change to a program that compiled before: **a clone's variants are
not in view as bare names**. `NameResolver::gather_variants` walked every enum
scope, clones included, so a bare `None` was *whichever clone another line of
the program had made* -- adding `Option<bool>` anywhere turned a working `let a
: Option<i32> = None` into an emitter failure. Record 0002 declares a clone
under a name no source can write precisely so that no lookup reaches it, and
this one did, by its variants. A clone's variant is reached through the
expected enum or through `Option<i32>.None`, never by lying around.

Several variants of one name in enums **the source wrote**, which was no answer
at all and reached the emitter in silence, is now the context's choice when it
names one (`let g : Fan = Off`) and otherwise an error naming them: *'Off' is a
variant of Lamp and Fan, so which has to be written before it*.

Cases: `tests/emitter/cases/a_generic_enum_is_built_from_its_variants` (42,
with a generic function over `Option<T>`, a `switch` taking one apart, and a
recursive `Seq<T>`), and
`tests/type_table/cases/a_generic_enum_is_solved_from_its_variant`, whose
golden holds each binding's clone and the four refusals. Seven sabotages, each
caught: the clone's variants left untyped, the expected enum ignored, clones
back in view, the generic name as a value giving nothing, the emitter naming
the enum from the left of the dot, no solving from the arguments, and several
variants left silent.

## Amended 2026-09-30: an enum writes methods, and `Option<T>`

Hadley, asked while putting `Option<T>` in the library: its API is **methods
on the enum** -- `found.unwrap_or(0)` -- so an enum may write methods. They are
a struct's: never virtual, since nothing derives from an enum (record 0065).

The parser had always read a `def` inside an enum, and nothing after it knew
one could be there:

- **the emitter wrote none of them**, so `a.is_some()` was a C++ error about a
  member that did not exist;
- **a `switch` took every member for a variant**, so one over an enum with
  methods said *does not cover is_some, unwrap_or*. Every walk over an enum's
  variants asks `AstQuery::get_variants` now, which is the members that are
  not a `def`;
- **`switch *this`** over a tagged union came out `*this.tag`, which C++ reads
  as `*(this.tag)` -- no program had switched over anything but a name.

A C++ `enum class` can hold no member function, so an enum that writes a
method is emitted in the **struct shape** even when no variant carries
anything: the tag, the union only when there is something to hold, the makers
and the comparison, and the methods declared inside. `carries_a_payload`
answers yes for one, which is what every other place already asks to decide
how a variant is named, compared and switched over.

**Not decided**: an enum that writes `init` or `destroy`. It is built by its
makers and not by a constructor, so what either would mean is open, and no
program writes one.

`std.option` holds `Option<T>`: `is_some`, `is_none`, `unwrap_or`, and `map<U>`,
a generic method so an Option nobody maps carries none of it. There is **no
`unwrap`**: a `None` has nothing to give back and the language has no way to
stop a program -- record 0030's natives have no `abort`. Nothing else in the
library changed (Hadley: only the type, for now), and a project puts it in view
by naming `std.option` in its prelude.

Cases: `tests/emitter/cases/an_enum_writes_methods` (42) and
`tests/programs/cases/an_option_may_be_empty`, which runs clean under the
address sanitizer with an `Option<String>`.

