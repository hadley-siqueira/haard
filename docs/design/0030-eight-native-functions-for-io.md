# 0030 — Eight native functions, and everything above them in Haard

Status: **decided**, 2026-09-05, and implemented. **Temporary on purpose**, and
the record says what replaces it.

| | |
|---|---|
| The emitter writes the body of eight functions, by **name**, and only inside the module named **`std.low_io`** | **decided**, amended 2026-09-08 |
| C's `stdio` and **not** C++'s streams, and the reason is the handle | **decided** |
| The native surface is **one character at a time**; everything above it is Haard | **decided** |
| Not a `native` keyword, because what a real foreign interface looks like has not been decided | **decided** |

## The problem

Nothing in Haard can reach a C library, so a program cannot print. Hadley asked
for a temporary way in, and named the shape: *if we can read one character and
write one character, everything else can be written in Haard.*

## The seam that already existed

A `def` whose body is `pass` already emits a **whole function** — signature,
prototype, mangled name, and an empty body:

```cpp
void h1_10___io_write(int8_t *h1_11_handle, char h1_12_c) {
}
```

So nothing had to be added to the language. The emitter fills the body in.

## Decision

**1. By name, inside `std.low_io`.** The emitter knows eight names and writes
their bodies when — and only when — the module they are declared in is named
`std.low_io` (it was `std.io` until the amendment below). The module is half
the match, so a `__io_write` written anywhere else
is an ordinary function with an empty body and not something the compiler
quietly takes over. The same trick already finds `String` in `Coercion`.

**Not a `native` keyword.** A keyword is a language decision, and what Haard's
foreign interface should look like has not been taken. This costs **nothing**
in the scanner, the parser or the Ast, and deleting it is deleting one function
of the emitter.

**2. C's `stdio`, not C++'s streams.** Hadley suggested `ofstream`/`ifstream`;
the handle is what rules them out. A `FILE*` is an **opaque pointer** and is an
`i8*` in Haard with nothing lost. An `std::ofstream` is an **object** — Haard
cannot name its type, know its size, or run its constructor.

And `stdout`, `stdin` and `stderr` are `FILE*` too, so **one set of primitives
covers the console and a file alike**. That is what lets a `File` written in
Haard print to the terminal, which is what Hadley asked for.

**3. Eight functions, one character at a time.**

```
__io_open_read(path) -> i8*     __io_read(handle) -> char
__io_open_write(path) -> i8*    __io_at_end(handle) -> bool
__io_close(handle)              __io_write(handle, c)
__io_stdout() -> i8*            __io_stdin(), __io_stderr()
```

Everything above them — `File`, `write` of a character and of a `char*`,
`writeln`, `console()`, `open_read`, `open_write` — is **written in Haard**, in
`lib/std/io.hd`, where it can be read and changed without touching the
compiler. That is the point of stopping at eight.

**4. `read` gives back a `char`, and the end of the file is a second
question.** C answers both at once — `fgetc` gives an `int` and `-1` — and
record 0018 will not have it: an `i32` does not compare to a `char` literal, so
`if c == '\n':` would not even be writable.

The rule pushed the design somewhere better. The value and the end are two
questions with two types, and `at_end` is asked **after** a read, the way C's
`feof` is.

## Consequences

- **`#include <cstdio>` is always emitted.** An unused include costs a program
  nothing, and asking would mean walking the whole call graph before the first
  line is written.
- **The emitter suite can test output.** A case's binary now runs with the
  build directory as its working directory — so a file it writes lands there
  and not in the repository — and what it printed joins the golden, above the
  exit status, when it printed anything.
- **Where the standard library lives is still record 0017's question.**
  `std/low_io.hd` is a file in this repository and a manifest or a roots table
  has to point at it.

## What found a bug on the way

`let e : f64 = 'w'` **passed in silence** and the emitter wrote
`double e = 'w';`. `ExpressionTyper::literal` handles three kinds of literal
and split them into two: a char literal fell into the float branch, so it was
accepted wherever a float was, and `let d : i32 = 'z'` was refused in words
that called it *a floating point literal*.

Three families now: an integer literal is any integer width, a float literal is
an `f32` or an `f64`, and **a char literal is a `char` and nothing else**.

The eleventh hole found by writing ordinary Haard and running it, and it was
found by trying to compare a character read from a file.

## What replaces this

A decided foreign interface. When it lands, these eight become ordinary
declarations of it, `Emitter::native_body` is deleted, and **nothing that
stands on them changes** — which is the whole reason the line is where it is.

## Amendment, 2026-09-08 — the module is `std.low_io`

The natives moved out of `std.io` and into **`std.low_io`**, and `std.io` now
holds `print` and `println`, overloaded on what they are given. Hadley asked
for it in those words: a program should print with

```haard
import std.io

def main : i32
    println("hello, world!")

    return 0
```

and not by writing `let out = console()` first.

Two things follow, and neither is a new decision:

- **The emitter matches on `std.low_io`** now. One string, one line, and the
  half-a-match reasoning above is untouched.
- **`print` is written on the natives directly, not on `File`.** `console()`
  builds a `File` with `new` and hands back the pointer, so a `print` written
  over it would leak one per call. The handle is what it needs, and
  `__io_stdout()` is the handle.

`std.file` is unchanged and still writes the same names on a `File`: a program
that writes to a file opens one, and a program that writes to the console
calls `println`. The split the record cares about — a native floor of single
characters, everything else in Haard — is exactly where it was; only the name
of the floor changed, so that the obvious name belongs to the half a program
actually calls.
