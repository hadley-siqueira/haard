# 0083 — What C++ leaves undefined, Haard does not

Status: **built**, 2026-11-04, on what Hadley decided on 2026-10-08 and 2026-10-20
(answers 1a, 2b, 3a, 7a, 9b and 10a in `notes/ir-questions.md`). The IR already did
all of it; this brings the C++ path to the same answers. Hadley asked for it:
*"pode seguir com o comportamento indefinido e a checagem de limites"*.

| | |
|---|---|
| A whole number wraps at its own width, signed or not | answer **1a**, built |
| A shift by a count at or past the width gives the arithmetic result; a negative count counts as too large | answer **2b**, built |
| A written shift count at or past the width is a compile error | answer **2b**, built, in the front end |
| A zero divisor stops the program with a message; `MIN / -1` is `MIN`, `MIN % -1` is 0 | answer **3a**, built |
| A written divisor of zero is a compile error | answer **3a**, built, in the front end |
| A float going to a whole number it does not fit, or a NaN, stops the program | Hadley 2026-10-20, built |
| Every index of a fixed array, `Array`, `List` and `String` is checked; a raw pointer never is | answer **9b**, built |
| `s[i]` exists on a `String`, reads and writes, and stops out of range; `String.at` keeps giving `'\0'` | **decided**, Hadley, 2026-11-04: *"adiciona s[i] checado em String"* |
| `-ffp-contract=off` and `-ftrivial-auto-var-init=pattern` | answers **7a** and **10a**, built as flags |

## Why it was urgent

The emitter wrote `a + b`, `a / b` and `x << n` straight into C++, and the tests
passed only because g++ ran without `-O`. Nothing could tell: at `-O0` a signed
overflow happens to wrap, so a golden proves nothing about it.

## How

**Arithmetic is computed where it is defined.** `a + b` over an `i32` is
`(int32_t)((uint32_t)a + (uint32_t)b)`. A narrow type goes through `uint32_t` too:
two `u16` promoted to `int` can overflow it, and the cast back is what cuts the
result, so `(a + b) as i32` with two `u8` at 255 and 1 is 0, where it was 256. This
replaces record 0081's `(char)(...)`. `u32`, `u64` and `usize` stay as written.

`-fwrapv` would have left `a + b` readable, and it was not taken: unlike
`-funsigned-char`, no `static_assert` can check it, so a build without it would be
silently wrong, which is the failure this record exists to end.

**`a += b` writes its place once**, as `(uint32_t&)a += (uint32_t)b`. Signed and
unsigned of one width may alias in C++. A narrow place needs no cast on the left.
`++` and `--` go the same way, and read back as the type they were.

**Division and shifts are helpers**, written only if a program reached them, as
record 0057's are. They work in the widest of their family and are cast back. A
written divisor or count, which the front end has already allowed, is C++'s own
operator. The compound forms hand their place over by address, as `//=` did.

**Everything that stops the program goes through `__abort_with`**, which flushes
stdout first, then writes the same message the interpreter gives to stderr.
`tests/lowering/lower.cpp` adds that message to what it compares, so the two paths
have to agree on the text as well as on 134.

**The intrinsic.** `__check_index(index, length)` in `std.low_io` is what
`Array.operator[]`, `List.operator[]` and `String.operator[]` ask. On the C++ path it is a native; in
the IR, `FunctionLowerer::call` turns it into `check_index`, the instruction a
fixed array's index already was.

**The two flags** cannot be written in the emitted file, and nothing checks them.
Every `Makefile`, the suites and the README pass them. A build that leaves them
out gets contraction back on a target with FMA and unspecified locals, not a
refusal. `-ftrivial-auto-var-init` needs GCC 12 or Clang 8.

## Found on the way

- `builtin_of` did not read through a reference. By its code, `r //= 2` with `r`
  an `i32&` would have failed as a type never worked out, which was not run. It
  reads through one now, and `whole_numbers_wrap` covers `r += 1` and `q //= 2`.
- `q - p` between two pointers is typed as an `i64`, so it took the new cast and
  came out in bytes. `a_pointer_moves_by_a_whole_number` caught it.

## Not covered

- **`Range.operator[]`** is not checked; answer 9b does not name it.
- **Only a written number is a constant.** `a / (1 - 1)` is decided when it runs,
  by the same answer. A negative written count is not refused, as answer 2b says.
- **The test cases' own copies of `std/`** do not ask for the check, except the
  three new ones. Nothing compares those three with `std/`, so a change to the real
  `operator[]` is not caught by them.
- **Null and a pointer past its buffer** stay undefined. A raw pointer is the
  unchecked way out.
- `new T[n]` buffers are not filled by `-ftrivial-auto-var-init`, which reaches
  locals only. Answer 10a holds, since reading them still gives some bit pattern,
  though the C++ standard does not promise that.

Cases: `tests/emitter/cases/whole_numbers_wrap`,
`a_shift_gives_the_arithmetic_result`, `division_of_the_lowest_by_minus_one`,
`a_zero_divisor_stops_the_program`, `a_float_that_does_not_fit_stops_the_program`,
`a_fixed_array_is_checked`, `a_negative_index_is_checked`,
`an_array_checks_its_index`, `a_list_checks_its_index` and
`a_string_checks_its_index`, all run through the IR
too by `tests/lowering`; `tests/type_table/cases/a_constant_that_cannot_be_meant`.
`tests/emitter/run.sh` builds the first three with `-O2` and the undefined
behaviour sanitizer, since a golden cannot see what they are about. Measured once,
not kept in the suite: all 85 programs of `tests/emitter` end the same under `-O2`
and under that sanitizer, plus its float conversion check.

Fifteen sabotages, each caught, counted only when a program's end or the sanitizer
changed, since a golden holds the C++ text and would fail on anything: `i32` not
computed unsigned, a narrow type not cut, `+=` without the unsigned place, `<<` and
`>>` not testing the count, `MIN / -1`, a zero divisor not stopped, an index
unchecked, a negative index let through, a float not checked, the IR's check
dropped, each of the two refusals off, `builtin_of` not reading through a
reference, and `String`'s check off in its case's copy. **Three of them (`+=`, `<<`, `>>`) only the sanitizer saw**, which is
why that check stays in the suite.
