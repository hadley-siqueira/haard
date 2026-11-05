# Native back end tests

Only needs `g++` and `bash`, and no `as` or `ld`. `hdc --emit-asm`
writes the text, `toolchain/hasm` assembles it with `toolchain/runtime`, and the program
runs with no C.

```
./run.sh
```

Record 0084 makes the two paths that exist the oracle, and that is what judges a program
here, by its output and exit code:

| | |
|---|---|
| `cases/` | written for this suite, one thing each, built and run **live** three ways: C++, the IR's interpreter and native |
| `widths.sh` | a generated program per whole number type: its edge values, each against each, through every operator, folded into the exit code; the same three ways |
| `../emitter/cases` | against what its golden says the C++ path did |
| `../programs/cases` | the same |

A program the back end does not write yet is counted, not failed, and the end of the run
lists why. **That list is what is left of step 3.** A case of this suite's own has to be
written. Anything else that differs fails.

Each program runs under a time and a file size limit: a wrong one tends to loop writing.
