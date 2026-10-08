# 0076 — A pointer moves by a whole number

Status: **decided and built**, 2026-10-07. Hadley accepted the recommendation
of the second probe round: pointer arithmetic as in C, the programmer trusted
(record [0047](0047-what-the-language-expects-of-the-programmer.md)).

| | |
|---|---|
| `p + n`, `p - n`, `p += n`, `p -= n`, with `n` any whole number type | **decided**, Hadley |
| `q - p` between two pointers of one type is the distance, an `i64` | **decided**, Hadley |
| Nothing else is arithmetic on a pointer; comparing two still is what it was | **decided while building it** |
| `n + p` is not written: the number goes on the right | **decided while building it** |

## Before

`p + 3` was *a literal cannot be i32**, the 3 asked to be the pointer. And
any arithmetic between two pointers of one type passed, typed as a pointer:
`q - p` was declared an `i32*` and g++ refused it, and `p * q` reached g++
too.

## How

In `ExpressionTyper::binary`, a number beside a pointer is typed as itself and
not asked to be the pointer, and the pointer rules come before record 0018's
*the two sides are one type*. In the statement checker, `p += n` and `p -= n`
type the `n` alone and ask it to be a whole number. `p++` already worked.

Cases: `tests/emitter/cases/a_pointer_moves_by_a_whole_number` (15) and
`tests/type_table/cases/a_pointer_has_only_plus_and_minus`. Five sabotages,
each caught.

Also pinned the same day: `c += 1` on a `char` stays refused (record 0018's
char family, Hadley), `tests/type_table/cases/a_char_has_no_number_arithmetic`.
