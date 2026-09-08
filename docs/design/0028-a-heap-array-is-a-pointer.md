# 0028 — `new T[n]` is a `T*`, and where the length lives

Status: **decided**, 2026-09-05, and implemented. Hadley, confirming C++'s
answer. Agenda 5.5 is closed alongside it, and it is the same record because
both were found by trying to write `String`.

| | |
|---|---|
| `new T[n]` gives back a **`T*`**, and the length is not in the type | **decided** |
| So `delete` and `delete[]` are the author's to tell apart | **decided** |
| The length is an **expression**, read off the tree, and is typed | **decided** |
| Agenda 5.5: nothing is generated; the question moves to the `TypeCollector` | **decided** |

## Decision

**1. `new T[n]` is a `T*`.** What C++ says, and the other two answers were
closed by decisions already taken.

`T[]` was the obvious alternative — an array value, which the compiler could
then require `delete[]` for. Record 0022 rules it out: `T[]` is sugar for
`Array<T>`, and `Array<T>` is the class you **build** with this. It cannot also
be what this gives back.

A third kind — a heap array that decays to `T*` — would buy the `delete[]`
check for a `TypeKind`, a decay rule on record 0018's closed list, and a case
in every phase. C++ does not make that check either, and this is a systems
language.

**2. The length is not in the answer, and that is the whole cost.** A `T*` does
not say whether it came from `new` or from `new[]`, so which of `delete` and
`delete[]` to write is the author's to know.

Record 0023 had already assumed this without saying so: `String` carries its
own `capacity`, which is only necessary if the pointer does not carry it.

**3. The length is read off the tree.** A type is a **value** — record 0016
interns it, and two `10`s must be one type — so `TypeBuilder` reads a length
only far enough to see whether it is an integer literal, and gives everything
else `NO_LENGTH`. The expression that wrote it is still a child of the type
node, and at a `new` it is an ordinary expression.

So **a non-literal length works**, and it works without agenda 5.3's constant
evaluation — which is about a length inside a **type** and still does not
exist. `new char[capacity]` compiles; `let buf : char[capacity]` does not.

**Nothing typed that expression until this record.** It lives under a type
node, and a type resolves no names, so `new char[n]` died in the emitter with
*"'n' names no declaration"* — the tenth silent hole found by writing ordinary
Haard and running it. A length that is not a whole number is now reported too.

**4. Agenda 5.5: there is nothing to generate.** Hadley asked whether a class
that lacks a default constructor should be given one that initialises its
object fields. Checked by running it: a class with **no `init`** already gets
C++'s implicit constructor, and that runs the constructors of its class-typed
fields down the whole chain. Record 0026 said as much — *"no `init` at all is
an aggregate, and C++ builds it for nothing"* — and it is true in practice.

A class that **wrote** an `init` taking arguments does **not** get a free
no-argument one. Generating it would make a written `init` stop guaranteeing
that it ran: `let x : Needs` would become valid with `n` holding whatever the
memory held, and the invariant the author wrote would be optional. C++ deletes
the implicit default constructor once any constructor is declared, for this
reason. Record 0026's rule 4 stands.

So the only thing 5.5 was ever about is **where the question is asked**. It was
asked in the emitter, which meant `hdc file.hd` called a program fine and
`hdc --emit-cpp file.hd` called it broken, and the refusal had no file, no line
and no caret — alone among this compiler's diagnostics. It is asked in the
`TypeCollector` now, where a declaration gets its type, which is also the one
place holding the node to point at.

```
error: every 'init' of Needs takes an argument, and none is written here
  --> main.hd:37:5
   |
37 |     field : Needs
   |     ^^^^^
```

All four ways a value comes into being with nothing written are covered: a
field held by value, a base, a local and a global. The base's message says
*"a base cannot be given one"*, because that is the case the author cannot fix
by writing arguments — `super(...)` is a decision record 0026 did not take.

## Consequences

- **`delete[]` needed nothing.** It was already whole — scanner, parser,
  `AST_DELETE_ARRAY`, pretty printer, typer, emitter — and the typer already
  refused `delete[] 5`. What it deletes could simply never be created.
- **`String` is writable in Haard**, which is what this unblocked, and record
  0023's `char*` → `String` becomes testable end to end for the first time.
  `tests/emitter/cases/a_heap_array_is_a_pointer` is that `String`, allocating,
  copying, indexing and freeing, and its verdict is the exit status.
- **The emitter's copy of the 5.5 question is gone.** The driver returns before
  emitting when any phase reported, so it was code against a state the design
  excludes.
