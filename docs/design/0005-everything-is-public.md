# 0005 — Everything a module declares is public

Status: decided, 2026-08-24

## Context

The symbol section of a serialized module already had to carry more than the
exports, because a generic body is instantiated in the importer and may reach
the declaring module's helpers (record 0002). That made visibility urgent: the
grammar has no `pub`, `private` or `protected`, and something had to decide
what a module offers.

## Decision

Haard has no visibility levels. Everything a module declares is public.

## Consequences

- **The symbol section needs no filtering at all.** There is no second, smaller
  "export list" next to it, and no rule about what a generic body is allowed to
  reach. What the module declares is what the blob holds is what an importer
  may name.
- **A module's whole surface is its API**, helpers included, so a star import
  brings in more names and collisions get likelier. That is subject 1.7.
- **It makes `interface_hash` coarser, and that has to be compensated in how it
  is computed** (subject 3.4). ~~*Retired by
  [0015](0015-the-cache-is-best-effort.md), 2026-09-02: there is no
  `interface_hash`. A `.hdm` is valid while its `.hd` has the same size and
  mtime, and a comment rebuilding every dependent is explicitly acceptable.*~~
  The paragraph below is kept because it records what the compensation would
  have had to be, if the question ever reopens. With everything public, the interface is every
  declaration in the file, so the hash must be taken over a *canonical form of
  the signatures* — not over the source bytes, or a comment would rebuild every
  dependent. And the split is not "declaration versus body": a **generic** body
  is part of the interface, because the importer instantiates it, while an
  ordinary body is not.
