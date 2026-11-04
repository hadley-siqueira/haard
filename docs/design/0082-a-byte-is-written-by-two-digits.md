# 0082 — A byte is written by two digits

Status: **decided and built**, 2026-11-04. Hadley: *"pode seguir só com \xHH,
mantendo \0"*. The other choice was C's octal in full.

| | |
|---|---|
| `\xHH` is one byte, with **exactly** two hexadecimal digits of either case, in a `char`, a string, a template string and a symbol | **decided**, Hadley |
| There are no octal escapes: `\303`, `\1` and `\012` are errors | **decided**, Hadley |
| `\0` is the byte zero, and only when no digit follows it | **decided**, Hadley |
| `'é'` is an error: a `char` holds one byte and this takes two | **decided**, Hadley, with the rest |
| `c == 0xC3` stays refused: a whole number is not a `char` | **unchanged**, record [0018](0018-implicit-conversion.md) |
| `\xHH` in a string is that raw byte; nothing checks that the result is valid UTF-8 | **follows** |

## Why

Record [0081](0081-a-char-has-no-sign.md) gave a `char` the range 0 to 255 and
left no literal for its upper half. `'\xC3'` was a `char*`, because the scanner
counted a backslash and one letter as an escape, leaving `C` and `3` as two more
characters. That was a gap and not a decision: the README said every other escape
was C's.

Looking at it found two places where the paths disagreed, in silence:

| Program | C++ path | IR |
|---|---|---|
| `"\xC3"[0] as i32` | 195 | 120: `unescape` dropped the backslash and kept the `x` |
| `let c : char = 'é'`, `c as i32` | 169: g++ kept the last byte | 195: the IR kept the first |

**Exactly two digits** because nobody then has to ask where the escape ends. C,
C++ and C# let it run on, so `"\xC3a"` is one byte too large there; D, Go, Zig,
Python and Rust's `b'...'` take two. **No octal** for the same reason: in C,
`\012` is one byte and `\08` is two. Rust, Zig, C# and Swift have none. This comes
from what is known of those languages and was not run. Zig is the closest.

**`'é'` is an error** and not its code point, as in Go and Zig, because 233 is not
the byte `é` is in a Haard string, and `'ő'` would not fit anyway. This does not
go against record [0047](0047-the-programmer-is-trusted.md): the compiler cannot
decide which byte was meant. `'é'` in double quotes stays a string of two bytes.

## How

- **`Scanner::get_escape`** is the one place an escape is read, used by strings,
  template strings and symbols, which each had a copy. It counts as one character
  and reports the two escape errors. `get_string` reports the `char` of more than
  one byte. `bootstrap/scanner/scanner.hd` does the same, byte for byte.
- **`FunctionLowerer::unescape`** reads `\xHH`.
- **`cpp_escapes`** writes `\xHH` as three octal digits, since C++ would otherwise
  take the `a` of `"\xC3a"` as a third digit. Three octal digits are where C++
  stops on its own. The symbol table had a copy of this function and now calls it.

Only `a_string_joins_orders_and_walks` changed its emitted C++ (`"\xff"` is now
`"\377"`). The IR had been reading that literal as the text `xff`.

Cases: `tests/emitter/cases/a_byte_is_written_by_two_digits` (run through the IR
too by `tests/lowering`); `tests/scanner/cases/escapes_are_one_character` and
`escapes_that_are_refused`, which `bootstrap/` runs too. Five sabotages, each
caught: the scanner without `\x`, a `char` of several bytes let through, the IR
without `\x`, the emitter leaving `\x` to C++, and one message changed in the
bootstrap scanner alone.
