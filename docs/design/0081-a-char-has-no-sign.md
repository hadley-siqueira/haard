# 0081 — A char has no sign

Status: **decided and built**, 2026-11-04. Hadley: *"haard char é sem sinal"*.
The comparison he read is `notes/should-a-char-have-a-sign.md`.

| | |
|---|---|
| A `char` is eight bits with no sign, 0 to 255, on every target | **decided**, Hadley |
| It stays a type of its own, distinct from `u8` | **unchanged**: overloads and `init`s tell them apart |
| `c as i8` is how the signed value is asked for | **follows** |
| `c ± n` wraps at eight bits on both paths, as record [0079](0079-a-character-moves-by-a-whole-number.md) said | **built on the C++ path**, which did not do it |
| The C++ file hdc writes refuses to compile if its `char` has a sign | **chosen in building it**, see below |

## Why

C++ leaves the sign of `char` to the platform, signed on x86-64 and usually
unsigned on ARM Linux, so the same Haard program could change its result between
two of Haard's targets. The IR already had no sign, and the two paths disagreed
on nearly every reading past 127: `'z' + 10` as an `i32` was -124 on one and 132
on the other. Without a sign, text orders as UTF-8's code points do, and a
character used as a table's index cannot leave the table.

## How

- **The IR**: nothing changed. It already widened a `char` with `zext`.
- **The C++ path**: a Haard `char` is still written as C++'s `char`, compiled
  with **`-funsigned-char`**, and every emitted file begins with

  ```cpp
  static_assert((char) -1 > 0, "a Haard char has no sign: compile with -funsigned-char");
  ```

  It holds on a platform whose `char` is already unsigned, where the flag is not
  needed. A build that forgets the flag stops there, so it cannot run with the
  wrong answers.
- **The wrap**: `Emitter::emit_binary` writes `(char)(a + b)` around `+ - * / %`
  when the result is a `char`. C++ computes in an `int`, so `(c - 1) as i32` with
  `c` at zero was -1 where Haard's eight bits say 255.

## Why not `unsigned char` in the emitted C++

It is what the note proposed trying, and it was measured. `uint8_t` is `unsigned
char`, so `init(c : char)` beside `init(b : u8)` becomes one constructor, which
g++ refuses (*cannot be overloaded*). A method and a function are safe, being
named by Haard's types, but C++ does not let a constructor be named. It would
also need a cast on every string literal, on `argv` and on `fopen`.

`char8_t` is distinct and unsigned, but needs C++20 where the Makefiles ask for
C++17, needs the same casts, and may not alias other objects the way `char` does.

The price of the flag is that **every build of an emitted file has to pass it**:
each project's `Makefile`, and anyone calling g++ by hand. The `static_assert`
is what makes that safe to ask.

## Not covered

- **A literal past 127** could not be written when this was built. Record
  [0082](0082-a-byte-is-written-by-two-digits.md) gave it `'\xC3'`. A whole number
  is still not a `char`, so `c == 0xC3` stays refused.
- **The other narrow types on the C++ path**: `(a + b) as i32` with two `u8` at
  255 and 1 was 256 in C++. Record [0083](0083-what-cpp-leaves-undefined.md) closed
  it, and replaced the `(char)(a + b)` above with the general form.
- `(data[i] as u8) as i32` in `std/string.hd` was written to get around the sign.
  It is still right, and `data[i] as i32` would now do.

Cases: `tests/emitter/cases/a_char_has_no_sign` (run through the IR too by
`tests/lowering`), and a check at the end of `tests/emitter/run.sh` that a build
with `-fsigned-char` is refused by the emitted file. Four sabotages, each caught:
the `static_assert` removed, the wrap off, the flag off in the suite, and the IR
widening a `char` with its sign.
