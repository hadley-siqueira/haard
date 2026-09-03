# Emitter tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout` — the same four
every other suite needs, and `g++` twice over: once for the compiler's own
sources and once for the C++ the compiler wrote.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project, the same shape `tests/compilation`
uses:

```
cases/<name>/
    table.tbl        the roots table, or neither
    entry            one line: the file to start from
    <the sources>    real Haard, run through the whole front end
```

## The last line of the golden is the suite

`run.sh` takes the C++ that came out, **compiles it with `g++` and runs it**,
and writes the exit status at the end of the golden:

```
--- exit: 145
```

That line is the point. The generated text is kept in the golden so a change to
it is visible and reviewable, but every tree over the same program can be
printed many ways and only one thing says the transpiler is right: the binary
runs and gives the right answer. A case whose output `g++` refuses gets
`--- did not compile` and the errors, which is the failure this suite exists to
catch.

A case the emitter **refused** has no program to build, and its golden is the
sentence it gave instead. That is a behaviour and not an absence: a kind the
emitter does not claim has to reach the reader as words, never as C++ that
compiles and does something else.

## What each case pins

| case | what it proves |
|---|---|
| `classes_inheritance_and_overriding` | 145, and every part of it is a decision. `Square.describe` overrides `Shape.describe` and is reached three ways — through a **base pointer**, through a **bare name inside an inherited method** (record 0020), and through a **`.` on a pointer** (record 0018's third coercion, which C++ does not have). `pick` is overloaded and the two calls choose different ones. Naming a method by its candidate, dropping `virtual`, or writing `.` where `->` was needed each move the number |
| `control_flow_and_operators` | 147: `while`, a C shaped `for`, `if`/`elif`/`else`, arithmetic, comparison, the bitwise four, `not`/`and` in their word spellings, and a declaring assignment. A wrong precedence or a loop that runs the wrong number of times moves it |
| `two_modules_that_declare_one_name` | 49. Two modules each declare `helper` and they are different code, which is what record 0010 puts in one program when two versions of one library are reached. Each is called from inside its own module, so no call is ambiguous. Emitting source names makes them **one** function in the C++ and the linker picks a body — the answer is then 40 or 90, so the exit status tells the two apart |
| `a_program_of_many_modules` | 79, from **four modules in one C++ file** — which is what a real program looks like. A class deriving from one in **another module** and overriding its method, a field of that base reached by a **bare name across the boundary** (which did not work until 2026-09-03), an **aliased** import and a **qualified** call, a field initialiser, an `init` with a defaulted parameter, and a **destructor** observed by writing through a pointer it was handed. The emitter orders `Shape` before `Box` although Box's module was found first |
| `a_class_that_cannot_be_built_says_so` | every `init` of a class needs an argument, so a value of it cannot come into being and the emitter refuses by name — rather than emitting C++ that `g++` refuses in words about a line nobody wrote |
| `what_cannot_be_emitted_says_so` | a hash literal has no meaning yet (agenda 1.23), and the emitter says so by name instead of writing something |

## Why the goldens are not sensitive to the mangled names

They are, and that is accepted. A name is `h<module>_<candidate>_<source>`, so
inserting a declaration renumbers everything after it and the goldens churn.
The alternative — hashing the name, or numbering it by something stable — buys
less than it costs: the indices are what make the name unique without a
lookup, and a golden that churns is a golden somebody reads.
