# 0079 — A character moves by a whole number

Status: **decided and built**, 2026-11-03. Hadley chose option (a) of four:
*"2a"*. The other three were `char - char` as a `char` with wrap, no
arithmetic on a `char` at all (`(c as i32) - ('a' as i32)`), and C++'s
promotion of every `char` to `int`, which record
[0018](0018-implicit-conversion.md) rules out.

| | |
|---|---|
| `c - d` between two characters is the distance between them, an `i32` with its sign | **decided**, Hadley |
| `c + n` and `c - n`, with `n` any whole number type, are a `char`, wrapping as eight bits do | **decided**, Hadley, asked when the first rule alone broke `ch - ('a' - 'A')`. It **overturns** his answer of 2026-10-07 (probe round 2), *a number is not added to a char* |
| `n + c` is not written: the number goes on the right, as with a pointer | **follows** record [0076](0076-a-pointer-moves-by-a-whole-number.md) |
| `c += n` and `c -= n` move it in place, as `c = c + n` does | **decided**, Hadley, with the row above |
| `c + d`, `c * d` and the rest between two characters are still a `char` | **unchanged**, and open |

## Why

`(c - 'a') as i32` is how a letter or a digit becomes an index, and it was the
one line two `String` programs of `tests/emitter` could not get through the IR
(*arithmetic on this type*). The front end had typed it as a `char` and the
C++ path got the right answer only because C++ promotes both sides to `int`
before the `as` is read. As a `char`, `'a' - 'b'` would be 255.

It is record 0076's shape, one type over: two pointers subtract into a
distance (an `i64`) and a pointer moves by a whole number. A character is a
position in a table in the same way.

The second rule was not in the question and had to come with the first. With
`'a' - 'A'` an `i32`, the upper case of a letter, `ch - ('a' - 'A')`, became
a `char` minus an `i32` and was refused, which is the line
`a_string_joins_orders_and_walks` is built on. Hadley was asked, since it
undoes his 2026-10-07 answer, and chose it over keeping that answer (and
writing `((ch as i32) - ('a' - 'A')) as char`) or going back on `char -
char`.

## How

- `ExpressionTyper::binary`: a number beside a character on its left is typed
  as itself, not asked to be the `char` (the branch the pointer takes); then
  `char ± whole` is the `char` and `char - char` the `i32`, before record
  0018's *the two sides are one type*.
- `StatementChecker`: `c += n` and `c -= n` type the `n` alone and ask it
  to be a whole number, as `p += n` does; another `char` on the right is
  what it was before.
- The IR (`FunctionLowerer::arithmetic`): `char - char` widens both with
  `zext` and subtracts in `b32`, the way `c as i32` widens one; `char ± n`
  cuts `n` to eight bits with `trunc` and adds in `b8` (`move_char`, which
  `+=` and `-=` use too).
- The C++ path needed nothing: C++ computes both in `int` and the emitter
  writes the binding with the type the front end gave.

## Not covered

A `char` past 127: the C++ path's `char` is signed on x86-64 and the IR's is
not (`c as i32` is a `zext`), so `'z' + 10` read back as an `i32` is -124 on
one path and 132 on the other. This is older than this record (any `as i32`
of such a `char` differs) and is in `docs/PENDING.md`. The case stays below
128.

Cases: `tests/emitter/cases/a_character_moves_by_a_whole_number` (63), run
through the IR too by `tests/lowering`;
`tests/type_table/cases/a_char_moves_by_a_whole_number`, which was
`a_char_has_no_number_arithmetic` and said the opposite; `a_string_joins_orders_and_walks` and
`a_char_pointer_reaching_a_string` came off `tests/lowering/not_yet` with it.
Four sabotages, each caught: `char - char` typed as a `char` again, the
IR's subtraction made an addition, the checker's `+=` rule off, and the IR's
`+=` on a `char` off.
