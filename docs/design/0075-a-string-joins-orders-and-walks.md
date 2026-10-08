# 0075 — A String joins, orders and walks

Status: **decided and built**, 2026-10-07. Hadley accepted the recommendation
of the second probe round. Adds to `std/string.hd` (record
[0023](0023-a-char-pointer-becomes-a-string.md)'s String); nothing that
was there changes.

| | |
|---|---|
| `a + b` gives a **new** String, neither side changed | **decided**, Hadley |
| `<`, `<=`, `>`, `>=` order by bytes, each read **unsigned**, a prefix first; `compare` gives below zero, zero or above | **decided**, Hadley |
| `s.contains(part)` finds a String inside, beside `contains(c)` for a char; an empty one is in every String | **decided**, Hadley |
| `for c in s` walks the characters, each a `char&` into the String (record [0040](0040-for-in-is-an-iterator.md)) | **decided**, Hadley |

A literal on either side works, since a string literal waits for the String
beside it (2026-10-07, the same probe round): `"ab" < a`, `a + "-"`.

The cursor, `StringCursor`, holds the String and not its bytes, as
`ArrayCursor` holds its Array: an `append` inside the loop may move them.

Case: `tests/emitter/cases/a_string_joins_orders_and_walks` (31), on a copy of
the library. Five sabotages, each caught: a signed compare, a `+` that
changes the left side, a search one short at the end, a cursor of copies, and
a prefix ordered after.
