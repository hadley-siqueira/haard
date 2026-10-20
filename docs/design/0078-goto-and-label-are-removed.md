# 0078 — `goto` and `label` are removed

Status: **decided and built**, 2026-10-20. Hadley, answering IR question 78
([notes/ir-questions.md](notes/ir-questions.md)): *"retira goto e label da
linguagem"*.

| | |
|---|---|
| `goto` and `label` are not in the language: no statement, no keyword | **decided**, Hadley |
| Both words are ordinary names again | **follows** |
| What replaces a `goto` that leaves several loops (a labelled `break`, say) | **open**, not proposed |

Supersedes record [0063](0063-what-the-front-end-refuses.md)'s row on labels.

## Why

The IR (record [0077](0077-an-ir-for-the-processor.md), question 78) has a
**structured** form -- regions `if`, `loop`, `for`, `block`, with `break` and
`continue` -- that the lowering produces and that loop bounds for the WCET,
vectorisation, CGRA kernels and function splitting work on, and a CFG form
below it. A `goto` could jump into a block, a loop included, and so write a
loop with two entries (**irreducible** control flow), which no nesting of
regions can hold:

```
if x:
    goto middle
label top
a()
label middle
b()
if c:
    goto top
```

Keeping `goto` meant a second way into the IR for any function using it,
lowered straight to the CFG form and kept out of every loop pass, with its
loops found by analysis and an irreducible one needing a bound written by
hand. Without it, every Haard function is structured, the lowering is
direct, and nothing has to be recovered from jumps.

## What it was

`label name` and `goto name` (record 0063): a `goto` named a label of its own
body, a label was written once, a backward jump was always fine and a
forward one could not land where a declaration it skipped was in view. The
C++ emitter wrote them as C++'s own `goto` and `name:;`. `break` and
`continue` never took a label.

## What changed

Removed from the scanner (two keywords), the parser, the Ast, the
AstBuilder, the pretty printer, AstQuery, the UseResolver, the
StatementChecker (the whole label check, and `declaring`, which only it
read) and the emitter -- in `src/haard/` and in the bootstrap's own copy in
`bootstrap/`, which `make check` holds to the same goldens.

Cases removed: `tests/parser/cases/label_simple`, `label_with_no_name`,
`tests/statement_checker/cases/a_goto_needs_its_label`,
`tests/use_resolver/cases/a_label_and_a_goto_name_nothing_here`,
`tests/emitter/cases/a_goto_reaches_its_label`. Goldens moved:
`tests/scanner/expected/keywords` and `tests/parser/expected/jump_every_kind`.

Case added: `tests/emitter/cases/goto_and_label_are_ordinary_names` (exit 7),
a function `goto` and a local `label`. Sabotage: `goto` put back as a keyword
fails it.
