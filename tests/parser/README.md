# Parser tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every file in `cases/*.hd` is scanned and parsed by `parse_and_print.cpp`, which
writes four things to `expected/<case>.txt`:

1. **the diagnostics**;
2. **the source the pretty printer writes back** from the tree. For a file that
   parses, this is the input again; for one that does not, it is what the
   recovery managed to keep;
3. **the tree**, one node per line, two spaces per level, with the lexeme of the
   token each node was built from;
4. **the round trip verdict**.

## Why the tree is dumped

The printed source is an in-order walk of the leaves and the operators, so
**every tree over the same sequence prints the same text**. Without the dump,
a parser that folded `a + b * c` into `(a + b) * c` still printed
`a + b * c` and the suite stayed green — measured, not guessed. The dump is what
makes precedence, associativity and the token behind each node visible.

## The line rule, and where it stops

A statement lives on one line, and record 0024 suspends that inside a bracket.
Four cases carry it: `brackets_span_lines` writes every bracketed form over
several lines with the closer opening a line of its own;
`a_closure_in_brackets_keeps_the_line_rule` puts a braced block inside those
brackets and the tree has to show **two** statements where a suspended rule
would give one sum; `an_unclosed_bracket_does_not_reach_the_next_statement`
says a bracket nobody closed does not swallow the rest of the file; and
`let_parenthesis_closed_on_next_line` used to pin the opposite answer and now
pins this one. The `*_on_next_line` cases around it are unchanged — none of
them opens a bracket, so none of them is joined.

## The round trip

The printed source is written to the build directory, parsed again, and the two
trees are compared as dumps — structural, since node indices and token offsets
differ between the two runs. Printing a tree and reading the result back has to
give the same tree, or the printer and the parser disagree about the language.
Two ways it can fail, both of which name the reason:

- `the printed source did not parse`, followed by the diagnostics. The printer
  wrote something that is not valid Haard;
- `the tree changed`, followed by the second tree. The printer wrote valid
  Haard that means something else.

It has to hold for the error cases too: what the recovery kept is a real program
and must parse cleanly on its own.

## Reading the golden diff

**`./run.sh -u` accepts whatever the parser produces, bugs included.** Step one
is running it; the step that gives the test its value is reading
`git diff expected/` afterwards. The round trip is the one part that fails on
its own rather than waiting to be read — a golden ending in anything but
`round trip: ok` is a bug that was written down, not a decision.

The per-case `timeout` is a test of its own: a parser that stops making progress
shows up as a failure instead of hanging the suite. Two of the cases here exist
exactly for that — a parser whose primitives do not go inert in panic mode loops
forever on them.
