# 0020 — The base chain in a lookup: inheritance, overriding, and virtual

Status: decided, 2026-09-02, by Hadley. Amends [0012](0012-overloading-and-per-module-tables.md).

## Context

Record 0012 made a name map to a list of declarations, and made the scope steps
— local, class, module, imports — **contribute to one candidate set** rather
than stop at the first hit. `ExpressionTyper::members_of` applied that to a
base: *"a method a base declares is an overload of one the derived class
declares, not something it hides."*

Taken literally, that made the most ordinary class in the language impossible:

```
class Shape:
    def describe : i32
        return sides

class Square(Shape):
    def describe : i32
        return 4
```

`sq.describe()` gathered **both**, with identical signatures, and reported
*"this call matches more than one 'describe' equally well"*. Overriding a
method did not work at all.

And a second hole, found next to it: inside a method of `Square`, the bare
names `sides` and `describe()` resolved to **nothing**. The scope chain is
lexical — a method's scope, the class body, the module — and a base is not
lexically around anything, so nothing ever looked in it.

## Decision

Hadley, 2026-09-02:

**A derived class sees what its bases declare**, by a bare name and not only
through a `.`.

**A method a derived class writes with the same parameters as one of its bases
overrides it**, and does not join it as an overload.

**Every method is virtual.** There is no `virtual` keyword and there will not
be one. A method is virtual because it can be overridden, not because it was
marked.

## Consequences

- **Record 0012 is amended at the class boundary only.** Its rule still holds
  everywhere else, and it still holds between a base and a derived class for
  methods with *different* parameters: those are two real overloads and both
  stay callable. Only the **same parameters** make one an override.
- **The return type is not part of *deciding* what overrides what**, because
  record 0012 keeps the return out of what makes two overloads different. It is
  checked separately, once the override is known, and the rule is C++'s:
  **the same type, or a pointer or reference to something derived from what the
  base gives back.** Covariance is for a pointer and a reference and never for
  a value — a caller through the base reserved room for the base, and the
  derived one does not fit. `OverrideChecker`, suite in
  `tests/override_checker/`. Hadley chose this over same-type-only on
  2026-09-02: same-type-only would have rejected a covariant `clone`, which
  already worked.
- **Two declarations in one class that tie are still ambiguous.** An override
  is about two classes; a class that declares the same method twice has not
  overridden anything.
- **The two halves live in two places, each where its machinery already is.**
  Gathering up the base chain needs no types, and is `NameResolver`. Deciding
  that one candidate overrides another needs signatures to compare, and is
  `OverloadResolver` — which is also the one place both ways of reaching a
  method end up, the bare name and the dot.
- **Virtual by default is a whole-program question, and it is not answered
  here.** Whether `Shape::describe` needs a slot depends on whether *any* class
  anywhere derives from `Shape` and writes `describe` — which the module
  holding `Shape` cannot know. A C++ emitter sidesteps it by writing `virtual`
  on every method, which is the semantics and costs a vtable pointer per class.
  Narrowing that to only-what-is-overridden is an optimisation that needs the
  whole program, and record 0015 already says what to do when the answer cannot
  be cached: **rebuild the module whole**. Hadley, 2026-09-02: *"if a cache can be
  had, good; if not, rebuild to be safe."*
- **A cycle in the bases would make the walk run forever.** Nothing rejects one
  yet (agenda 1.10 decided type cycles by value, not this), so `gather_bases`
  stops at a class it has already reached. That is not a diagnostic and does
  not pretend to be one.

## Not decided here

**`init` and `destroy`.** Hadley, 2026-09-02: the constructor is the method
`def init : void` and the destructor `def destroy : void`, by convention and
not by keyword. Nothing in the front end needs them — they are ordinary methods
and already collect, type and resolve. What they force is all in the emitter:
when `init` runs for `let c : Counter`, when `destroy` runs, what a class with
neither does, whether `def init : i32` is a mistake worth reporting. That is
its own record, and it will be written when the emitter asks the questions.
