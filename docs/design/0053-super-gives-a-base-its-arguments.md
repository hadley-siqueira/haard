# 0053 — `super` gives a base its arguments

Status: **decided and built**, 2026-09-09. Hadley named the syntax.

| | |
|---|---|
| `super(...)` calls the base's `init`, picked by the arguments written | **decided**, Hadley |
| It may be written **anywhere** in an `init`, not only first | **decided while writing it** |
| A class with a base that needs arguments must write it in **every** `init` | **decided while writing it** |
| A class whose `init`s all take arguments gets a **do-nothing C++ default constructor** | **decided while writing it** |

## What was wrong

Record 0026 runs a base's `init` with nothing wherever a derived value comes
into being, so a class whose every `init` took an argument **could not be
derived from at all**:

```
error: every 'init' of Base takes an argument, and a base cannot be given one
```

The message named the problem and no way out, and the code that wrote it said
why: *record 0026 leaves `super(...)` undecided, so there is nowhere to write
a base's arguments even when the author knows them.* That sentence is what
this record retires.

## The syntax

```haard
class Square(Shape):
    side : i32

    def init : void
        @length : i32

        super(4, 2)

        side = length
```

`super` is a keyword — zero collisions in the repository — and it is only ever
a **callee**. The parser reads it as a primary and the postfix rule wraps it
into the call it is written as, so nothing in the grammar knows a call is
coming.

It names nothing the resolver could find: what it means is the base of the
class this body is inside, which is a question about the **scope**. So it is
answered from there, and that is also what makes it an error outside a class
and an error in a class that derives from nothing. The arguments are then
handed to record 0026's own `initialisation` — the same one `new Base(...)`
uses — so they are ranked, reported and recorded by machinery this record did
not touch.

## Where it may be written

**Anywhere in the `init`, not only as its first statement.** A `super` inside
an `if` is a program a reader can follow, and refusing it would be the
compiler guessing at intent — record 0047. What C++ guarantees about the base
running first is kept a different way, below.

**But every `init` of the class must write one**, when the base cannot be
built with nothing. A class that declares no `init` at all is still refused,
because there is nowhere to put one, and the diagnostic says so:

```
error: every 'init' of Needs takes an argument, and no 'init' of this
       class writes 'super(...)'
```

## What it compiles to

```cpp
void h0_6_Square::m_init_b6(int32_t h0_9_length) {
    this->h0_1_Shape::m_init_b6_b6(4, 2);
    this->h0_7_side = h0_9_length;
}
```

**Qualified**, for the reason record 0026's constructors name their class: an
unqualified call would reach an override and not the base's own.

And the base sub-object is built by C++ **before** this body runs, with a
do-nothing default constructor the emitter now writes for a class that
declares no `init` answering to no arguments. `super` fills it in afterwards.
Leaving the fields alone until then is exactly record 0026's rule about a
field with no value written.

## What writing it found

**`holder_of` gives back a declaration's node, and `name_of` wants its
candidate.** Handing the node straight over named whatever candidate sat at
that index — which in the first test was the derived class itself, so the
`init` called itself and the program overflowed its stack. It ran before it
was read: the case's exit status was a segmentation fault, which is the one
kind of wrong answer the emitter suite cannot miss.

**"Takes no parameters" is not "answers to no arguments".** The first draft of
the do-nothing constructor counted written parameters, so a class with
`init(@count : i32 = 3)` — which record 0012's arity-as-a-range makes answer
to none — got a second, empty constructor beside the one C++ writes with the
default. The two were ambiguous, which **deleted the implicit default of every
class derived from it**, in C++ and not in Haard. Counting *required*
parameters is the fix, and it is the same count `OverloadResolver::required_of`
has always used.

## What is not decided

**Whether a base's `init` should run before the derived body's first
statement.** Today it runs where `super` is written, and a body that reads a
base's field before calling `super` reads whatever C++'s default constructor
left. That is record 0026's rule about uninitialised fields, applied one class
up, and nothing reports it.

**`super.method()`** — reaching a base's *method* rather than its
constructor. Not written, not needed yet, and the syntax is free.
