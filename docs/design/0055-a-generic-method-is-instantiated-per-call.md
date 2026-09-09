# 0055 — A generic method is instantiated per call

Status: **decided and built**, 2026-09-09. The other half of record 0054.

| | |
|---|---|
| A type argument list is read after a `.` and a `->` too | **built** |
| The clone of a generic method lives **in its class's body** | **decided while writing it** |
| It is named in C++ by the name record 0002 already made unique | **found by reading the C++** |
| A method a program never calls is **not emitted**, like a generic class | **built** |
| A list written where there is no call is **refused** | **decided while writing it** |

## What was missing

Record 0054 gave a generic **function** its instantiation and left the method
half untouched, because `b.wrap<i32>(3)` did not **parse**: the type argument
list was read after a bare name and never after a dot. What came out was the
relational level's own complaint, which is true and about the wrong thing:

```
error: a comparison is written with spaces around it; glued, this opens a
generic argument list, and what is in front of it cannot take one
```

The parser side is one branch, shared with the bare name — `parse_generic_name`
now wraps either, and the spacing rule still decides: glued opens a list,
spaced is a comparison. `tests/parser/cases/generic_call_after_a_dot.hd` writes
all three in one file.

Everything else was a hole behind it, and each one was silent in a different
phase.

## The clone goes into its class's body

`Instantiator::instantiate` appended every clone to the module **root** and
collected it into the module scope. For a class that is right. For a method it
makes a free function: it loses the `this` its body reads, the emitter writes
it outside the struct, and the call that asked for it looks for a member the
class does not have. The C++ that came out was

```cpp
return h0_6_b.class(3);
```

— `class`, because the name node the emitter fell back to carries token 0 and
token 0 is the first word of the file.

So the clone goes **beside the declaration it was cloned from**: the type body
of its class, and that body's scope. Record 0002's two rules are untouched —
the clone lives in the module that **declared** the generic, under a name no
source can write — and this is only about which scope in that module.

## The statement checker kept its set by root declaration

Record 0054's round exists because a clone can be born in the middle of the
checking phase: `return f<i32>(3)` is only ever seen there. The checker keeps
the declarations it has walked so a second round costs nothing, and it kept
them **by root declaration**.

A method clone does not grow at the root. It grows inside a class the phase has
already walked, so it was never reached, its body was never typed, and the
emitter said *'x' names no declaration* about a parameter written two lines
above it. `has_more` and the round both ask about members now: the class stays
done — walking it again would report everything in it twice — and the new
member is the round's subject on its own.

**Only a call the statement checker sees needs this**, which is why the first
version of the test case did not prove it: every call inside a `let` is made
while the type phase walks, and the class is walked after. The case writes one
inside an `if` now, and taking the round out fails it.

## A clone is named by the name record 0002 already made

A method is named in C++ by its own name and its parameters and by nothing
else — record 0020, so that a derived class's override carries the same C++
name as the base's. A clone is a copy of the declaration, token and all, so

```haard
def first<T> : T
    @xs : T*
```

instantiated at `i32` and at `i64` gave **one** C++ name, `m_first_pb6`, for two
methods of one class differing only in what they give back. g++ refuses that,
which is the tenth time reading the emitted C++ has been the thing that found a
bug.

The name that is already unique is the one in the symbol table: record 0002's
unwritable `first#7`, the argument by its type index in the module that
declares it. That module is the one this is always asked with — the definition
and every call site both name the clone through its own module — so the two
agree. The cleaner turns the `#` into an underscore and the C++ reads
`m_first_7_pb6`.

Only a **method** takes its name this way. A free function is named by its
module and candidate indices and was unique already, so record 0054's output is
byte for byte what it was.

## A generic method nobody calls is not a method

`emit_bodies` walked a class's members without asking, so a generic method
wrote a body naming a type parameter nothing bound: *this type cannot be
emitted yet*, about a method the program never mentions. It is skipped now for
the reason a generic class is — it is not a method, it is what one is made
from — and `Emitter::is_generic` already told the two apart, since a clone is
registered as an instantiation and the original is not.

## A list with no call is refused

The parser reads a glued list after **any** dot, so it can be written where
nothing will be instantiated:

```haard
let held = b.take<i32>      # no call to rank it against
let field = b.n<i32>        # not even a method
```

There is no value in this language that is a method, so neither names
anything. Both are refused, at the name, with *a type argument list belongs to
a call, and this is not one* — rather than resolving to the member and dropping
the list, which would make the second of those a field read and two written
words nobody looked at.

**What is not refused**: a list written on a call to something that is not
generic, `b.plain<i32>()`. `instantiated` leaves a candidate with no type
parameters exactly as it found it, on purpose and since record 0054, so that an
ordinary overload of the same name still competes. It is silent about the list,
for a free function and for a method alike; if that is to change it is one
decision about both.

## What it cost

Six places, none of them large: the parser's branch, `callee_of` and
`name_of_callee` unwrapping the name, `member` refusing it, the instantiator's
choice of scope, the checker's round, and the emitter's name and its two skips.
Nothing downstream learned that a method can be generic — the clone is an
ordinary method in an ordinary body, which is record 0002's whole argument
arriving somewhere new.

`tests/emitter/cases/a_generic_method_is_instantiated_per_call` runs a program
that instantiates two methods of a plain class at four types, a generic method
of a **generic class** through a pointer, and one from inside an `if`; its
verdict is the exit status. `a_generic_method_crosses_a_module` is the same
question across a module boundary, where the clone stays in the module that
declared it and the app names it correctly.

## What is still open

A generic method **without** written type arguments — `b.first(xs)` — needs the
same unification a free `f(3)` needs, and neither exists. That is one subject
for both.
