# 0019 — The resolution table: what each node turned out to be

Status: decided, 2026-09-02. Closes the open half of agenda 2.8.

## Context

Every expression kind types. `ExpressionTyper` answers what an expression is,
and `NameResolver` and `OverloadResolver` answer which declaration a name
means. **Both answers are used and thrown away.**

That was fine while the only consumer was a diagnostic, which is produced on
the spot. It stops being fine the moment anything downstream needs to know:

- a **C++ emitter** has to write a declaration's type at a `let`, has to know
  which overload a call chose (C++ would re-resolve it, and C++ *has* implicit
  conversions, so it may choose differently), and has to know whether a bare
  name is a local, a parameter, a field of `this` or a global in another
  namespace
- agenda **3.3** wants generic bodies with their names pre-resolved
- anything that walks the tree a second time would have to re-run the typer,
  and the typer **reports as it goes** — so a second run means every
  diagnostic twice

## Decision

**One entry per Ast node, in a table beside the tree, holding what that node
turned out to be.**

```
struct Resolution { u32 type; u32 module; u32 candidate; }
```

- `type` is an index into **this** module's type table, `INVALID_TYPE` for a
  node that is not an expression or whose type could not be worked out
- `module` and `candidate` are the declaration the node names, and `0` for a
  node that names none

It is `ResolutionTable`, per module, beside `StringTable`, `SymbolTable` and
`TypeTable` — the fourth of the family and the same shape as the other three:
one flat vector, index 0 a sentinel, nothing owned, a read past the end giving
back the sentinel the way `Ast::get_node` already does.

**The typer writes it, and nothing else does.** `ExpressionTyper::type_of` is
one funnel that every expression goes through, so recording the type is one
line at the end of it rather than a line per kind.

## Consequences

- **The pair `(module, candidate)` is written down for a third time**, after
  record 0016's named type. Record 0013 made it transient for a *lookup*; that
  still holds. What is stored here is not a lookup, it is an **answer** — which
  declaration this particular written name meant — and no lookup can recover it
  afterwards, because overload resolution used the argument types to pick.
- **In the blob it becomes a dependency index**, the same conversion agenda 3.2
  owes record 0016. One more field on that list and no new problem.
- **A literal argument has to be filled in after the call is resolved.** Record
  0018 carries a literal into a call untyped so that each candidate may ask it
  to be its own parameter, and `OverloadResolver` checks the digits itself
  without going back through the typer. So the literal's node is never typed on
  the way in, and its type is only known once a candidate has won: the call
  records it then, from the chosen signature.
- **It is dense, and that is on purpose.** A map keyed by node would be smaller
  for a file that is mostly declarations, and would break the one discipline
  records 0013 and 0016 both keep: a section of the blob is a `resize()` and a
  `memcpy`, and nothing in it is a pointer.

## Rejected

**Recording on the Ast node itself.** `AstNode` is 13 bytes and the parser
writes it; hanging semantic results on it would make the tree mean different
things before and after the type phase, and would put the parser's output and
the typer's output in one structure that no phase owns alone.

**Recomputing on demand.** The typer reports as it goes, so a second run is a
second set of diagnostics. Making it silent on demand means two modes of one
class, and the mode that stays silent is the one no test would be watching.
