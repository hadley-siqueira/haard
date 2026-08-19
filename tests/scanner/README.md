# Scanner tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, from the build directory: `make check`.

Exits with code 0 if everything passed, 1 otherwise.

The output is verbose and printed as the work happens, so nothing looks frozen:

```
[6/9  66%] CC    scanner.cpp
[9/9 100%] LD    check_invariants

[ 1/21   4%] PASS  blank_lines
[ 2/21   9%] PASS  char_vs_string
```

Every compilation step and every case gets a line prefixed with a
`[done/total percent]` counter, so it is clear how much is left. `PASS` is green,
`FAIL` and `XPASS` red, `XFAIL` yellow; when a case fails, the diff or the
invariant message is printed indented right below it.

On a terminal, the step currently running is shown first on a transient line
(`RUN`, `CC`, `LD`) that its own result then overwrites — that is what makes a
slow or hanging step visible while it is still running. In a pipe or a log file
only the result lines appear, without any escape codes. Colors also turn
themselves off with `NO_COLOR=1` in the environment.

Most of the wall time is the build, not the tests: the scanner sources take a
few seconds to compile, while all the cases together run in well under a second.
The sources shared by the two drivers are compiled to objects once and linked
twice, instead of being handed to `g++` once per driver.

## How it works

Every file in `cases/*.hd` goes through two drivers, both compiled on the fly
from the scanner sources:

**`dump_tokens.cpp`** prints the token stream in a stable format, one line per
token, with kind, offset, length, line bit, indentation counter and lexeme. The
output is compared against `expected/<case>.txt` — that is the **golden test**.
Scanner error messages come out before the tokens and are part of the comparison
too, so the diagnostic text is covered.

**`check_invariants.cpp`** checks properties that must hold for any input,
without depending on an expected output — this is what catches regressions in
new files, for which nobody has written an `expected/` yet:

- *round-trip*: each token's lexeme is exactly `source[offset, length)`, the
  tokens are in order, do not overlap, and whatever separates two tokens is only
  whitespace or a comment. No code byte is lost or duplicated.
- *whitespace*: the indentation counter matches the spaces at the start of the
  line where the token begins; tokens on the same line share the flag bit;
  tokens on different lines have it flipped.

The per-case `timeout` is a test as well: a scanner that does not advance shows
up as a failure instead of hanging the suite.

## Adding a case

1. Create `cases/my_case.hd`
2. Run `./run.sh -u`
3. **Read the `git diff` of `expected/`** and confirm the output is right
4. Commit both files

Step 3 is the only one that matters: a golden test is only worth something if
someone checked the gold. `-u` blindly accepts whatever the scanner produces,
bugs included.

## Known failures

`known_failures.txt` lists cases that document bugs not fixed yet. They show up
as `XFAIL` and do not break the suite. If a listed case passes, the suite reports
`XPASS` and fails — that way the file cannot go stale silently.

A listed case still goes through `-u` like any other. If a case ever gets an
`expected/` file written by hand — holding the output the scanner *should*
produce, which is what would make it fail — say so here and refresh it by hand
instead, because `-u` would turn the documented bug into the golden.
