# Parser tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every file in `cases/*.hd` is scanned, parsed and printed back as source by
`parse_and_print.cpp`. The output — the diagnostics first, then the tree — is
compared against `expected/<case>.txt`.

**For a file that parses, the expected output is the input again.** That round
trip is what makes the test worth something: it checks the tree the parser built
and the printer's view of it at once. For a file that does not parse, the
diagnostics come first and the tree that follows shows what the recovery
managed to keep — which is the part worth reading when a case changes.

**Read the golden diff.** `./run.sh -u` accepts whatever the parser produces,
bugs included. Step one is running it; the step that gives the test its value is
reading `git diff expected/` afterwards.

The per-case `timeout` is a test of its own: a parser that stops making progress
shows up as a failure instead of hanging the suite. Two of the cases here exist
exactly for that — a parser whose primitives do not go inert in panic mode loops
forever on them.
