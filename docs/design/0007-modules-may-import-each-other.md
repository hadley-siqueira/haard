# 0007 — Modules may import each other, and two passes resolve it

Status: decided, 2026-09-01

## Context

Subject 1.3 asked whether a cycle between modules is an error or a supported
shape. Every earlier record assumed it was an error: the recursion sketched for
4.1 was `parse(A) -> ensure_checked(dep) -> check(A)`, which pauses A, compiles
B to completion, writes B's blob and resumes A. That only terminates on a DAG.
Rejecting cycles is also what Rust and Zig do across compilation units, and it
was the recommendation on the table.

## Decision

**Cycles are allowed.** `A` may import `B` while `B` imports `A`, and neither is
an error.

They are resolved by splitting the semantic phase in two:

1. **Collect declarations** for every module in the cycle. After this pass every
   module's signatures exist, so a name from a cyclic partner can be found.
2. **Check bodies**, with every declaration already visible.

## A module cycle is legal; a type cycle is not

The two passes make `A` and `B` visible to each other. They do **not** make a
type able to contain itself:

```
struct A:
    b : B

struct B:
    a : A
```

is an error, because the size of `A` needs the size of `B` which needs the size
of `A`. The rule is C++'s: a cycle may only be closed through indirection —
`A*`, `A&`, `A***`, `B&` are all fine, a field by value is not.

So the semantic phase is three stages, not two: collect declarations, **compute
layout** (which is where a type cycle is detected and reported), then check
bodies.

## A module may import itself

A cycle of one. It is legal and it brings in nothing new, since a module's own
declarations are already in its scope — the only thing it can add is a qualified
form for them, when written as `import a as p`. **It warns**, because no real
code has a reason to write it and the likelier explanation is a mistake.

## Consequences

- **The unit of compilation stops being the module.** `B` cannot be finished and
  serialized before `A` when `B` needs `A`, so the unit becomes the group of
  modules that reach each other — a strongly connected component of the import
  graph. A module that is in no cycle is a group of one, which is the common
  case and keeps the simple path simple.
- **A group is cached and invalidated together.** The `.hdm` and the cache key
  of subject 3.4 are per group; a source edit anywhere in a cycle rebuilds every
  module in it. There is no way around this — their declarations were resolved
  against each other.
- **Subject 4.1 has to be written in terms of groups**, not modules: find the
  imports, build the graph, condense it, and run the three stages per group in
  topological order of the condensation.
- **`CHECKING` no longer doubles as the module cycle detector**, which is what
  record 0004's sketch used it for. Re-entering a module that is `CHECKING` is
  now the normal case. The state that still detects a cycle is the layout stage,
  and what it detects is a type cycle.

## Rejected

Rejecting cycles with a diagnostic naming the cycle path. It buys a simpler
scheduler — every module finishable in isolation, one blob per module, a cache
key per file — and it costs the user a real expressive limit that C++ and Java
do not impose. The two-pass split is well understood and the scheduler cost is
paid once.
