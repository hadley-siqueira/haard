# 0056 — A generic may derive from a generic

Status: **decided and built**, 2026-09-09. It replaces a refusal decided and
withdrawn the same day, and the reason it was withdrawn is the record.

| | |
|---|---|
| `class Derived<T>(Base<T>)` **works**, and needed no new machinery | **measured** |
| What stopped it was **one character** in `TypeCollector::walk` | **found by measuring** |
| A refusal was decided first, on a cost estimate that was **wrong** | **withdrawn** |
| Fixing it uncovered a refused `for ... in` leaving a local behind | **fixed** |

## The refusal that was withdrawn

The first version of this record refused the shape by name. The argument was
that record 0002's machinery *cannot* make a clone of it — that the derived
clone would have to instantiate its base before its own members were collected
and that the two clones could not be made to agree — and that no program was
asking for it, since record 0054 had just moved `Array<T>::equals` into a free
function for a related reason.

The first half of that was never measured. It is false.

## What actually happens

A use of a generic name is a **clone** (record 0002), and the clone carries a
copy of the written super. It resolves that same `Base<T>` in **its own
scope**, where record 0002's instantiator has bound `T` to a type — so it
builds an ordinary instantiation, `Base#i32`, and everything above it is
ordinary inheritance. The declaration this was written on top of never needed
to work: nothing reads the *original's* super, because nothing ever reads an
unbound generic.

`TypeBuilder` does give back the generic declaration written down when an
argument is still a parameter, which is what the original's super is. That is
the same branch `Array<T>&` inside Array's own body goes through, it is right
there for the reason its comment gives, and it was never the problem.

## The one character

```cpp
if (candidate <= done || candidate >= count) {
    continue;
}
```

`done` is the last index the pass **finished**, and a round walks what came
after it. Written `<=`, every round skipped exactly one candidate: the first
new one, at index `done` itself. It had been there since generics landed.

Most declarations survive it — a clone's methods and fields sit past the
boundary and are reached, and `type_signature_now` types a fresh clone's
signature on the spot for the caller that asked. A **class** clone is the
declaration that lands on the boundary, and `super_of` runs nowhere else. So
its `Candidate::super` was never built at all and stayed `INVALID_TYPE`, which
is how the rest of the compiler spells *no base*:

```
error: cannot find 'value' in this scope        # a field the base declares
error: expected Base<i32>*, found Derived<i32>* # the upcast
```

Two diagnostics, neither of them about anything the source got wrong, and both
one missing line of work apart from disappearing. With the guard fixed, the
same program emits `struct Derived : public Base`, inherits, dispatches through
the vtable and runs.

## What the fix uncovered

One case in the type table suite went from one diagnostic to two, and the extra
one was real: a `for x in t` over a class that answers no `iterator` reports
*Thing has no member named 'iterator'* from the typer, and then **built the
lowering anyway** — declaring the cursor local `let __c0 = t.iterator()`. That
local is a candidate of its own, so the collector reached it and typed the same
call a second time.

It read as one error only because the local was the candidate the off-by-one
skipped. `ForEachLowerer::over_a_cursor` refuses now when the cursor call has
no type: the loop is left standing, which is what every other refusal in that
pass does, and no local is declared. The container's sibling is put back as it
was — `call_on` makes the synthetic name its sibling, and the `in` node still
holds the container, so a walk of the standing loop would otherwise reach a
name this pass invented and report that it names nothing.

## What this says about the next one

The refusal was cheap to write and would have been permanent: a shape refused
by name is a shape nobody tries again. What it cost to find out that it was
unnecessary was one instrumented print in `super_of` — the clone had no line at
all, which said *never computed* rather than *computed wrongly*, and that is
the whole difference between the two records.

**Measure the mechanism before deciding it cannot work.** Record 0047 says the
compiler refuses when it cannot decide; this is the same rule one level up —
refuse a feature when the design cannot carry it, not when a phase has a bug.

`tests/type_table/cases/a_generic_may_derive_from_a_generic` holds a two-level
chain and its goldens are the types; `tests/emitter/cases/a_generic_derives_from_a_generic`
runs one, with an override reached through a base pointer, and its verdict is
the exit status.
