# hdc, in Haard

The Haard compiler written in Haard. **Only the scanner so far**, and it
passes every case of the C++ scanner's suite byte for byte.

It is laid out as a real Haard project and built the way one is: a manifest
written by hand, `hdc` writing the whole program as one C++ file, and a C++
compiler turning that into a binary. Nothing here is special to the compiler
repository except where `std` and `hdc` are found.

```
make            build build/dump_tokens
make test       build it and run it against the C++ scanner's goldens
make cpp        stop after the C++, to look at it
make table      build with roots.tbl as well, and check both give one program
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
| `tools/dump_tokens.hd` | `hdc.tools.dump_tokens` | `tests/scanner/dump_tokens.cpp` |

`haard.pkg` is the only file written by hand to build it: `hdc` reads it, and
the one it reaches in `../std`, and builds the roots table itself (record
0044). `roots.tbl` is that same table written out, for a build that passes
`--roots`. `make table` writes the program both ways and fails if they differ,
and `make test` runs it first, so the table cannot go stale in silence.

`token/token.hd` was **generated once** from `src/haard/token/token.h`, the
enum and its names, and is edited by hand since. Its kinds are the C++ ones in
the same order, and `name_of` writes each as the C++ dump does.

## The oracle is the C++ scanner

`tests/run.sh` has no `expected/` of its own. It runs `dump_tokens` over every
file in `../tests/scanner/cases` and compares the output with
`../tests/scanner/expected`, which the C++ scanner's suite already holds. So
the answer to every case was written down before a line of this was, and by
the scanner this one has to agree with.

The dump is the same byte for byte: rustc-shaped diagnostics first, sorted by
offset, then one line per token with its kind, offset, length, line and column,
newline flag, indentation and text. That covers the scanner and also the
`SourceFile`'s positions and the `Logger`'s rendering, tabs, utf8 and carets
included.

It is part of `make check`, which makes it a regression test of the
**compiler** as much as of the scanner: it is the largest Haard program in the
repository, and a change to `hdc` that breaks a real program shows up here.

## What writing it found

Four things, three of them bugs in `hdc` fixed in the same change, and one
question about the language that became record 0062:

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

## What is next

The parser, as the twin of `src/haard/parser/` and judged the same way: the
parser suite's goldens are the tree written back by the pretty printer, so the
bootstrap's parser needs its pretty printer before it can be compared with
anything.

`tests/scanner/check_invariants.cpp` has no twin yet. It checks properties of
any input rather than a written answer, and the goldens already cover every
case it runs on.
