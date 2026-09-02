# UseResolver tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project — an `entry` naming the file to start
from, the sources, and a `table.tbl` when the case has imports. Most have no
table, because most of the rules about what counts as a use are about one file
and only the qualified forms need two.

**The golden is the diagnostics.** This is the first pass in the name family
that produces one: until now the resolver answered questions a test asked, and
nothing asked.

## How it knows which scope it is in

It reads `Scope::owner` backwards. The collector opened one scope per function,
block, closure, for-each and type body and stamped the node that opened it, so
the walk switches scope when it steps into a node that owns one. There is one
description of the scope shape in the compiler and this pass is not a second
copy of it — a sabotage that drops the switch fails 4 of the 7 cases.

## What counts as a use

Decided by structure, not by a list of exceptions, so that it does not rot as
the grammar grows:

- **an `AST_BINDING_NAME` is a declaration wherever it appears.** Every
  declaration wraps its name in one — a function, a class, a field, a
  parameter, a binding, a closure parameter — so one rule covers them all and
  a new declaration written the same way is covered for free.
- **`AST_GENERIC_PARAMETERS` declares**, and so do a for-each's loop variables.
- **The right side of an `AST_DOT` is not resolved.** It is a field of whatever
  the left side turns out to be, so it needs a type. Sending it through the
  scope chain would find a global of its name in silence, which is the trap
  this pass exists to not fall into. `the_right_side_of_a_dot_is_not_resolved`
  reports `missing` in `missing.field` and nothing about `field`, `whatever`,
  `deeply` or `nested`.
- **An `AST_SCOPE` is one of the two qualified forms**, and goes to the
  resolver's entry points for them. `the_qualified_forms` reports
  `cannot find 'absent' in 'p1'` and `cannot find 'known' in 'nope'`, while
  `p1::known` and `::here` resolve — the second one from inside a class that
  has a field named `here`, which is what `::` is for.
- **A label and a goto name a namespace this table does not model**, so neither
  is resolved and `goto never_written` is not reported.

`a_use_may_come_before_its_declaration` pins that the module scope has no
ordering rule: a function may call one written below it.

## Three sabotages that do not fire, and why

| sabotage | fails |
|---|---|
| every use is resolved from the module scope | 4 |
| the right side of a dot is resolved through the scope chain | 1 |
| a label and a goto are resolved | 1 |
| an `AST_SCOPE` is walked as ordinary children | 1 |
| a binding name is walked, so every declaration is a use | **0** |
| generic parameters are walked | **0** |
| loop variables are not noted, so they are uses | **0** |

The last three are correct rules that this suite **cannot** prove, and the
reason is one fact: **a declared name always resolves in the scope it is
declared in.** Walking a declaration's own name finds the declaration and
reports nothing, so the diagnostics are identical either way.

They still matter, and here is where: a later pass that *records* what each use
resolved to would record every declaration as a use of itself. Until such a
pass exists, read these three rules as intent and not as tested behaviour —
which is the same honesty the compilation suite's "parsed alone" line is kept
with.

## One consequence worth a decision

`a_loop_binds_and_the_sequence_is_a_use` includes a C shaped
`for j = 0; j < 10; j = j + 1:` and it reports **`cannot find 'j'` five times**.

That follows correctly from what was decided: only a `for ... in` binds, and a C
shaped head is an assignment, not a declaration. So `j` has to be introduced by
a `let` before the loop. It is written into the golden rather than smoothed over,
because if that is not the intended language then this is the case that says so.
