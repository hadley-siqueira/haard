# 0070 — `unwrap` stops the program

Status: **decided and built**, 2026-10-07. Hadley chose, of three options for
`Option.unwrap`, the one that **stops the program with a message** when there
is no value, and asked that `unwrap_or` stay -- which it already was. Amends
record [0030](0030-eight-native-functions-for-io.md): a tenth native.

| | |
|---|---|
| `Option.unwrap` gives the value of a `Some`, and on a `None` stops the program with a message on the error stream | **decided**, Hadley |
| `unwrap_or(fallback)` stays as it was | **decided**, Hadley |
| The stop is `std.low_io`'s `__abort`, which is C's `abort()`: exit status 134, nothing cleaned up | **decided while building it** |
| What was printed before comes out before the message | **decided while building it** |

## Why a native

It is the first way a Haard program stops on its own: there was no exit and
no abort, so `std/option.hd` said in its header that `unwrap` could not be
written. Record 0030's natives are the one place Haard reaches C, by name and
only inside `std.low_io`, so the stop is one more of them:

```haard
def __abort : void
    pass
```

whose body the emitter writes as `fflush(stdout); fflush(stderr); abort();`.
`abort()` flushes nothing itself. It needs `<cstdlib>`, which is included only
when a program reaches `__abort` -- so every other program's head is
unchanged.

`abort()` and not `exit(1)`: an unwrap of a None is a bug in the program, not
an answer it gives, and `abort()` is what C and C++ stop on one with. Nothing
is destroyed on the way out, as in C++.

## What `unwrap` is

Ordinary Haard in `std/option.hd`:

```haard
def unwrap : T
    while true:
        switch *this:
            case Some(value):
                return value

            case None:
                __unwrapped_none()
```

The `while true` never runs twice, since `__abort` does not come back; it is
what tells record 0063's check that the method cannot reach its end without a
value. There is no way yet to say that a function does not return.
`__unwrapped_none` flushes the standard output, writes `error: unwrap called
on a None` to the error stream one character at a time, and calls `__abort`.

Case: `tests/emitter/cases/unwrap_stops_the_program`, with a trimmed copy of
the library: a `Some` unwraps, `k` is printed, a `None` stops it, and the
golden is the `k`, then the message, then `exit: 134`. Four sabotages, each
caught: `__abort` doing nothing, `<cstdlib>` never included, the message
before what was printed, and `unwrap` coming back from a None.

Copies of `std/option.hd` inside other test cases are their own (the
library's rule) and are not changed.
