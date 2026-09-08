# 0027 — What a bare name on the left of `=` declares, and what a derived class may not redeclare

Status: **decided**, 2026-09-05, and implemented. Found by writing ordinary
Haard and running it, which is the fourth session running that this is how a
blocking bug was found.

| | |
|---|---|
| `let` stays optional: `n = 1` declares n when nothing already in view is called n | **unchanged** |
| "In view" includes a field of a **base**, which is what it never did | **decided** |
| So the decision moves out of the symbol phase and into the **type** phase | **decided** |
| A derived class that declares a field a base already declares is an **error** | **decided** |

## The bug this record exists because of

Record 0020 made a bare name inside a method reach a field of a base. Reading
one worked from that day. **Writing** one did not:

```haard
class Base:
    n : i32

class Derived(Base):
    def read : i32
        return n      # Base.n, since record 0020

    def write : void
        n = 7         # a NEW LOCAL, and the field is never written
```

The C++ said it plainly — `void Derived::m_write() { int32_t h0_8_n = 7; }` —
and nothing was reported, because from the compiler's point of view nothing was
wrong: a name nothing declares, assigned to, declares itself.

## Why it could not be fixed where it was

`SymbolCollector::in_view` climbed the chain of enclosing **scopes**. A base is
not lexically around anything, so it was never on that chain — the same gap
record 0020 closed in the lookup, in a **second, independent** lookup that
never heard about it.

And it could not simply be told: a base is a `super`, which is a **type**, and
types are filled by the pass *after* symbols are collected. At the moment the
question was asked there was no answer to give — and worse, the base may live
in a module the symbol walk has not reached, or be an instantiation that record
0002 has not yet cloned.

This is the same rule every phase in this compiler has run into, one level
down:

> a question that reads across declarations cannot be answered inside the walk
> that fills them.

## Decision

**1. `let` stays optional.** An assignment to a bare name that nothing in view
declares declares it. Requiring `let` was the other way out and would have
deleted the question instead of moving it; Hadley kept the language as it is.

**2. The decision moves to the type phase.** A new `ImplicitCollector` runs
inside the round loop of `Compilation::collect_types`, after `TypeCollector`
has filled every `super` and instantiated every generic, and asks the
`NameResolver` — which knows bases, knows instantiations, and crosses a module
boundary by interning the name where it lands.

A name it declares is typed by the next round, which is what that loop already
existed for: record 0002's instantiation appends declarations to a module the
loop has already passed, and both kinds of growth are absorbed the same way.

**3. It still does not ask the imports.** That was deliberate before and stays
deliberate: a local standing in front of an imported name is what a written
`let` does too. So `NameResolver` gained `resolve_in_module` — the same walk
with the imports left off — and what a base declares is very much part of it
while what an import brings is not.

**4. A derived class may not declare a field a base declares.** Hadley,
2026-09-05.

Two fields of one name give the object both. Record 0020 makes a bare name in
a method of the **base** still mean the base's, so `n` written in a method of
`Base` and `n` written in a method of `Derived` read two different pieces of
memory, and nothing in the source says which one any given line meant. There
is no rule that makes that useful, so it is an error and not a shadowing rule.

It is checked in the `OverrideChecker`, which is where the base chain is
already walked and which already runs late enough to have one. A **method** of
a base's name is untouched: that is an override, and record 0020 decides it.

## What this does not decide

Whether a **local** may shadow an inherited field — `let n = 1` inside a method
of `Derived` — is not this record's. It is written, so it is not the mistake
that this record is about, and today it shadows.
