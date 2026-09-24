# hdc, in Haard

The Haard compiler written in Haard. **The scanner and the parser so far**,
with the pretty printer the parser is judged by, and they pass every case of
the C++ compiler's scanner and parser suites byte for byte: 454 of 454.

It is laid out as a real Haard project and built the way one is: a manifest
written by hand, `hdc` writing the whole program as one C++ file, and a C++
compiler turning that into a binary. Nothing here is special to the compiler
repository except where `std` and `hdc` are found.

```
make            build build/dump_tokens and build/parse_and_print
make test       build them and run them against the C++ compiler's goldens
make cpp        stop after the C++, to look at it
make table      build with roots.tbl as well, and check both give the same programs
make clean
```

`make HDC=/path/to/hdc` when the compiler is not at `../build/hdc`.

## The layout

Each module is the twin of a directory of `src/haard/`, and inside each one the
functions are in the C++ one's order, so that a difference between the two
dumps points at one function here and its namesake there.

| file | module | the C++ it is the twin of |
|---|---|---|
| `token/token.hd` | `hdc.token.token` | `src/haard/token/` |
| `source/source_file.hd` | `hdc.source.source_file` | `src/haard/source_file/` |
| `log/logger.hd` | `hdc.log.logger` | `src/haard/log/` |
| `module/module.hd` | `hdc.module.module` | `src/haard/module/`, only what the scanner needs |
| `scanner/scanner.hd` | `hdc.scanner.scanner` | `src/haard/scanner/` |
| `ast/ast_node.hd` | `hdc.ast.ast_node` | `src/haard/ast/ast_node.h` |
| `ast/ast.hd` | `hdc.ast.ast` | `src/haard/ast/ast.h` |
| `ast/ast_builder.hd` | `hdc.ast.ast_builder` | `src/haard/ast/ast_builder.cpp` |
| `parser/parser.hd` | `hdc.parser.parser` | `src/haard/parser/` |
| `pretty_printer/pretty_printer.hd` | `hdc.pretty_printer.pretty_printer` | `src/haard/pretty_printer/` |
| `tools/dump_tokens.hd` | `hdc.tools.dump_tokens` | `tests/scanner/dump_tokens.cpp` |
| `tools/parse_and_print.hd` | `hdc.tools.parse_and_print` | `tests/parser/parse_and_print.cpp` |

A tool is a program of its own: a file in `tools/` with a `main`. The
`Makefile` builds each one from the same modules.

`haard.pkg` is the only file written by hand to build it: `hdc` reads it, and
the one it reaches in `../std`, and builds the roots table itself (record
0044). `roots.tbl` is that same table written out, for a build that passes
`--roots`. `make table` writes the program both ways and fails if they differ,
and `make test` runs it first, so the table cannot go stale in silence.

Some files were **generated once** from their C++ twin and are edited by
hand since: the two enums (`TokenKind` from `token.h`, `AstNodeKind` from
`ast_node.h`) with the names the dumps print, the builder's makers, the
printer's one-line printers and its dispatch. What was generated is a table;
every function with a decision in it was translated by hand.

## The oracle is the C++ compiler's own suites

`tests/run.sh` has no `expected/` of its own. Each tool is run over every case
of the suite its twin is judged by, from that suite's directory, and compared
with the goldens that suite already holds:

| suite | tool | what a golden holds |
|---|---|---|
| `tests/scanner` (45) | `dump_tokens` | the diagnostics, then one line per token |
| `tests/parser` (409) | `parse_and_print` | the diagnostics, the source printed back, the tree, and the round trip |

So the answer to every case was written down before a line of this was, by
the C++ phase this one has to agree with. The round trip is the parser
suite's own: the printed source is written to a file, scanned and parsed
again, and the two trees have to be the same one -- which is the bootstrap
writing a file and reading it back through its own scanner.

It is part of `make check`, which makes it a regression test of the
**compiler** as much as of the bootstrap: it is the largest Haard program in
the repository, and a change to `hdc` that breaks a real program shows up
here. Every case also runs clean under AddressSanitizer and UBSan; that is
checked by hand, not by the suite.

## What writing it found

The scanner found four things -- three bugs in `hdc` fixed in the same
change, and one question about the language that became record 0062:

- **A class holding a `String` was copied shallow when assigned.** A `Log`
  with a `String` message, stored in an `Array<Log>`, was a double free the
  first time the Array grew. The emitter writes `m_assign` for a class that
  owns something (record 0031) and never C++'s `operator=` (record 0034), so
  a class that owns nothing itself but holds a field that does was assigned by
  C++'s implicit `=`, which copies the field's pointer. It now gets an
  `m_assign` of its own, field by field. Pinned by
  `tests/programs/cases/a_value_holding_a_string_is_copied_whole`.
- **An assignment through a reference was shallow too.** `logs[j] = logs[j -
  1]` asked whether a `Log&` owns something, and a reference owns nothing. The
  emitter reads through the reference now (record 0035). Same case.
- **`hdc --pkg haard.pkg` did not work**; `--pkg ./haard.pkg` did. The
  manifest's directory was the empty path, and a root at the empty path covers
  no file. The `Makefile` here writes the bare name, which is what keeps it
  fixed.
- **`let x = xs[i]` bound a reference**, not a copy: `operator[]` gives back a
  `T&`, and `let` took the type it was given. `let i = line_starts[low]`
  followed by `i += 1` walked the table itself forward. **Decided by Hadley
  the same day** as record 0062: a binding with nothing written is a copy, as
  C++'s `auto` is, and `let x : T& = ...` is how a reference is asked for.
  The code here writes it the plain way again.

The parser found one more bug, and two things worth knowing:

- **An enum taken by a method of a class written before it did not
  compile.** The emitter declared every class ahead of the types, and no
  enum: the order it writes the types in follows fields and bases, not
  parameters. The AstBuilder, written before `AstNodeKind`'s module was
  reached, took a kind in almost every maker. Every enum is declared with the
  classes now. Pinned by
  `tests/emitter/cases/an_enum_is_declared_before_a_class_takes_it`.
- **`"${"` cannot be written as a plain string.** It is read as a template
  string, and an unterminated one. `"\${"` prints `${`, but by accident: `\$`
  is no escape Haard has decided, and C++ is what turns it into a `$`, with a
  warning. The code here writes the two characters apart instead. An open
  question, not changed.
- **`Array` has no `pop`.** The parser's indentation stack keeps a depth of
  its own over an Array that only grows.

## What is next

The phases after the parser, in the order `hdc` runs them: the module loop
and `ModuleFinder` (which needs the file system, the one bootstrap blocker
left), the symbol table, the name resolver and the type table. Each is judged
by its own suite's goldens, the way these two are.

`tests/scanner/check_invariants.cpp` has no twin yet. It checks properties of
any input rather than a written answer, and the goldens already cover every
case it runs on.
