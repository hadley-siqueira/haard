# 0069 — `~=` is not an operator

Status: **decided and built**, 2026-10-07. Hadley chose to remove it, after a
survey of what other languages read `~=` as. Amends record
[0057](0057-the-precedence-of-the-operators.md), which listed `~=` among the
assignments and left what it does undecided.

| | |
|---|---|
| `~=` is removed: the scanner reads `~` and `=` as two tokens, and `a ~= b` is a syntax error | **decided**, Hadley |
| Clearing bits is written `x &= ~m` | **decided**, Hadley |

## Why

The scanner had a token for it and the parser an assignment kind, and nothing
had ever said what it did: `~` takes one operand, so `x ~= m` had no `x = x ~
m` to be short for. What other languages read it as did not agree:

| | `~=` |
|---|---|
| Lua, MATLAB | **not equal** |
| D, Raku | append |
| Odin | xor assign |
| Swift | pattern match (`~=` is the operator `case` calls) |
| Go | no `~=`; `&^=` is bit clear |

A reader coming from Lua or MATLAB would read `if a ~= b:` as a comparison,
and the bit clear it could have been is already `x &= ~m`. So it is not an
operator at all, which is also what leaves the spelling free.

## What it is now

```haard
a ~= b          # error: nothing may follow a statement on its line
if a ~= b:      # error: expected ':', found '~'
x &= ~mask      # clearing the bits of mask
```

The token, the AST kind and every case naming them are gone from the compiler
and from the bootstrap, which mirrors the token set.

Cases: `tests/scanner/cases/integer_division.hd` reads `~=` as two tokens now,
and `tests/parser/cases/tilde_equal_is_not_an_operator.hd` (the old
`assignment_bitwise_not`) is the error. One sabotage, putting `~=` back in
the scanner's table, caught by both suites.
