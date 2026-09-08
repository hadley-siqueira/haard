# 0026 — `init` and `destroy`: when they run, and what a class with neither gets

Status: **decided**, 2026-09-03, and implemented. Agenda 1.18 and 5.2, which
record 0020 opened and deferred with the words *"it will be written when the
emitter asks the questions."* The emitter asked them.

| | |
|---|---|
| `init` is the **constructor** and `destroy` the **destructor**, and both stay ordinary methods | **decided** |
| Arguments reach `init` through **`new T(...)`**; `T(...)` as a value comes later | **decided** |
| A field with no value written is **not initialised**; one with a value is | **decided** |
| A base whose every `init` needs an argument makes the derived **unbuildable**, and that is reported | **decided** |
| The destructor is **virtual**, always | **decided** |

## Decision

**1. They are ordinary methods, and they are emitted twice.** Record 0020
settled the names by convention and not by keyword, and nothing in the front
end treats them specially — they collect, type, resolve and overload like any
method, and `c.init(5)` is a call anyone may write.

The emitter writes each of them **twice**: once as the method it is, and once
as a C++ constructor or destructor whose whole body is a call to it. The call
is **qualified** — `this->Shape::m_init(...)` — because a virtual call from
inside a constructor reaches the class being built and not the one that will
exist, which is one of C++'s traps. Naming the class means the body that runs
is the one that class wrote, and the base's own constructor has already run
its own.

**2. Arguments reach `init` through `new T(...)`.** That form already parsed
and **nothing checked it at all** — `new Counter(2.5)` against an `init` taking
an `i32` passed in silence, and so did arguments to a class with no `init`. It
now resolves against the class's `init` candidates like any other call, with
record 0018's rules and record 0019's recording.

**Its own `init` and never a base's.** A base's runs on its own, before the
derived's body, the way every language with a constructor chain does it. So a
class that declares no `init` takes no arguments even when the class above it
does.

`T(...)` as a value expression is **not** decided here and is additive: it
needs the overload resolver to consider a class's `init` when the callee names
a class, and nothing that compiles today would change.

**3. A field with no value written is not initialised.** Hadley, 2026-09-03. It
is a systems language and not paying for what was not asked for is record
0018's line. A field with a value written — `n : i32 = 7`, which already parsed
and checked and which the emitter **silently dropped** — is initialised, and it
is written as a C++ default member initialiser so that it holds for every
constructor and for the implicit one a class with no `init` gets.

**4. A value comes into being running its class's `init` with no arguments.** A
local, a field held by value, a global, and the base of a class being built.
A class whose every `init` needs an argument cannot do that, and **there is no
syntax for writing a base's arguments** — so it is reported, by name, rather
than emitted as C++ that a compiler would refuse in words about a line nobody
wrote.

Record 0012's arity-as-a-range is what keeps this from being a wall: an `init`
whose every parameter has a default answers to no arguments, and that is how a
class that wants to be derived from is written today. `super(...)`, or the
C++ initialiser list, is a language decision this record does not take.

**5. The destructor is virtual, on every class, whether or not a `destroy` was
written.** A pointer to a base is how a program holds an object here — record
0020 makes every method virtual for the same reason — and deleting through one
without a virtual destructor runs the wrong code, silently and only sometimes.

## Consequences

- **`delete` is typed**, which nothing did: `delete 5` passed in silence. Its
  operand must be a pointer and it gives back void, being written for what it
  does like a call whose answer is thrown away.
- **A default parameter value now reaches the C++.** It was dropped, so record
  0012's arity-as-a-range produced a call the language allows and C++ had no
  function for. Written at the declaration only, which is where C++ takes one.
- **Global variables are emitted**, which they were not: they were referenced
  and never defined, so nothing linked. **In source order**, and that is a
  limit: C++ initialises a file from the top down, so a global whose value
  reads one declared below it reads that one's zero.
- **`def init : i32` is not reported.** A constructor giving something back is
  meaningless and nothing says so yet. Left open on purpose: it is one check
  and it belongs with the override checker's family, not here.
- **Two questions live in two places, and they are not the same question.** The
  typer asks *"do these written arguments match an `init`?"* at a `new`. The
  emitter asks *"can this class be built with no arguments at all?"* at a
  declaration, a field and a base. They overlap only at zero arguments. Moving
  the second into the `TypeCollector`, where a binding gets its type, would put
  a caret under the name and is the right next step.

## What this found

Three holes, all by writing a program of four modules and running it — the
lesson of 2026-09-02 and 2026-09-03 for the third time:

- **A field inherited across a module boundary was invisible to a bare name.**
  Record 0020 worked inside one file and nowhere else. The lookup skipped its
  whole scope walk when the importing module had never interned the name, which
  is right for its own scopes and wrong for the base chain — a base lives in
  another module with a string table of its own.
- **`new T(...)` was checked by nothing.**
- **`delete` was typed by nothing.**

## In code

`ExpressionTyper::initialisation` and `constructors_of`; `Emitter::emit_structors`,
`emit_globals` and `require_default_construction`; the `interned` test moved off
the loop in `NameResolver`.

Cases: `tests/emitter/cases/a_program_of_many_modules` — four modules, one C++
file, 79, with inheritance **across modules**, an aliased import, a qualified
call, a field initialiser, a defaulted `init` parameter and a destructor
observed through a pointer it was handed; `a_class_that_cannot_be_built_says_so`;
`tests/type_table/cases/an_allocation_checks_its_init`.
