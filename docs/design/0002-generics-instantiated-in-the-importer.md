# 0002 — Generics are instantiated in the importer

Status: decided, 2026-08-24

## Context

A generic can be compiled once in the module that declares it (Java, Go) or
instantiated in each module that uses it (C++, Rust). The choice decides
whether the serialized module has to carry its own ast.

## Decision

Generics are instantiated in the importer. The serialized module carries its
ast.

## Consequences

Three of them, all larger than the decision itself:

1. **The symbol section cannot hold exports alone.** Instantiating `B.map` in
   `A` means resolving the names inside `map`'s body, and those names live in
   B's scope — including B's private helpers. Everything reachable from an
   exported generic goes in the blob, private or not, so "the interface" and
   "what the blob holds" stop being the same thing.

2. **The body is resolved in B's scope, never in A's.** Resolving it in the
   importer is what makes a name inside `map` see the importer's declarations,
   which is the hygiene bug that C++'s two-phase template lookup exists to
   avoid.

3. **So the names in a generic body are pre-resolved to symbol indices when B
   is compiled**, and the blob stores the body already resolved, with only the
   type parameter left open. The importer instantiates by following indices
   and looks up no name at all.

Consequence 3 is what keeps the identifier text of a dependency out of the
blob: if the body carries indices instead of names, nothing in the importer
needs to read B's identifiers. See 3.1 and 3.3.
