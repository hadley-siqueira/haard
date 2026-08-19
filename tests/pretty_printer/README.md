# Pretty printer tests

No frameworks. Only needs `g++` and `bash`.

    ./run.sh

Or, together with the scanner suite, from the build directory: `make check`.
Exits 0 if everything passed, 1 otherwise.

## How it works

The pretty printer walks an ast, and **nothing builds one yet**:
`src/haard/parser/parser.cpp` is a half written refactor and does not compile.
So `test_pretty_printer.cpp` builds the ast by hand, node by node, the way the
parser is meant to build it, and checks the text that comes out.

The tokens are real: `cases/declarations.hd` is scanned first, and the nodes
point at its tokens by index — the comment above each test lists the ones it
uses. That is what keeps the checks honest about `print_node_token`, which reads
the lexeme out of the source file.

One `Context` per test: the ast is append only and `print()` always starts at
node 1, so each test needs a tree of its own.

## When the parser lands

These tests keep their value, but the one worth adding then is the round trip:
scan and parse a file, print it, and check that parsing the output again gives
the same tree. That covers the whole vocabulary at once, instead of a tree per
case built by hand.
