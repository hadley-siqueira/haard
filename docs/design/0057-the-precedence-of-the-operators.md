# 0057 — The precedence of the operators

Status: **decided and built**, 2026-09-10. It writes down an order that was
only ever in the parser's shape, changes two of its levels, and fixes the back
end, which had never carried any of it.

| | |
|---|---|
| The order is **deliberately not C's**: `**`, `|`, `^`, `&` and the shifts bind tighter than `*` | **kept, and written down** |
| The emitter wrote **no parentheses**, so C++ regrouped every one of them | **fixed** |
| `not` is a **loose word** above the comparisons and `!` is a tight symbol below them | **changed** |
| `**` is **right associative**, the way a power tower is read | **changed** |

## The order

Seventeen levels, loosest first, one function of the parser per level. The
table is in [../../README.md](../../README.md) and is not repeated here; what
this record keeps is why three of the levels are where they are.

**The bitwise operators and the shifts bind tighter than `*` and `/`.** That
is the old compiler's order and it is the reverse of C's, where all four are
looser than `+`. C's order is the one that makes `a & MASK == 0` a famous bug,
and it is a historical accident: `&` and `|` were the only spellings of `&&`
and `||` when the table was written. Haard's answer is that a bitwise operator
is arithmetic on the representation and binds like arithmetic that is tighter
still, so the parentheses C needs are not needed here.

**`as` is looser than everything except assignment.** Its left operand is the
whole expression to its left, and nothing may follow it — there is no level
between `as` and assignment for an operator to be read at, and `a as i32 + 1`
is a parse error rather than a grouping nobody would guess.

**A comparison does not chain.** `a < b < c` is `(a < b) < c` and the left
half is a `bool`, which the type phase reports. Python's chaining is a rule
about the *sequence* of comparisons and not a precedence, so it is not
something this cascade could express by moving a level.

## `not` is not `!`

Until this record `not` and `!` were one operator with two spellings, both at
the unary level, and `not a < b` was `(not a) < b`: it typed `not` against an
`i32` and reported a line nobody meant to write.

They are two operators now. The word sits between `and` and the comparisons,
which is where Python puts it, and takes the whole comparison to its right.
The symbol stays at the unary level, which is where C puts its own `!`, and
takes the operand next to it.

```haard
not a < b        # not (a < b)
!a < b           # (!a) < b
```

Two spellings that mean different things is a cost, and it is paid on purpose:
a reader of `not a < b` reads English, and a reader of `!a < b` reads C. The
alternative — one loose operator with both spellings — would make `!` mean
something no C programmer expects, and the alternative to that was the state this
record replaces, where the word meant something no English reader expects.

`not in` is unaffected. It is two tokens combined at the **relational** level
and is only ever met with a left operand already in hand, so `a not in b`
never reaches the word's own rule and `not a not in b` is the negation of
`a not in b`.

## `**` is right associative

`a ** b ** c` is `a ** (b ** c)`. The left folding reading would be a
redundant spelling of `a ** (b * c)`, and no notation spends a syntax on
something it can already say — which is why mathematics reads a power tower
from the right, and why Python, Ruby, Fortran, Haskell, F#, Perl, R and
JavaScript all do. The old compiler folded it left; that was a mistake and not
a decision.

The interaction with the unary minus is **not** changed here: `-2 ** 2` is
`(-2) ** 2` in Haard and `-(2 ** 2)` in Python. Python gets that by giving
`**` a right operand at the unary level and a left operand at the primary
level, which this cascade cannot express without also changing what `- a | b`
means. It is open.

`**` is still parsed and printed and **not typed**, so none of this is visible
in a program that runs yet. The parser suite dumps the tree, which is where it
is proven.

## The back end had none of it

Record 0025 transpiles to C++, and C++ reads five of these levels in a
different order. The emitter wrote each binary operator as `left oper right`
with nothing around it, so the C++ compiler regrouped every expression the two
languages disagree about:

```
println(4 + 3 & 1)      the tree says 5      the program printed 1
println(1 << 2 + 3)     the tree says 7      the program printed 32
println(2 * 3 | 4)      the tree says 14     the program printed 6
```

No diagnostic anywhere: `hdc` was right, g++ was right, and the program was
wrong. Every suite was green, because no test had ever written an expression
the two languages read differently — which is exactly how a defect of this
shape survives.

The fix is the classic one for unparsing a tree into a language with its own
table: each operand is asked what precedence **C++** would read it at, and
wrapped when the answer is looser than the operator holding it. It is C++'s
table in `Emitter::precedence_of` and one guard in `emit_operand`, and it adds
a pair of parentheses only where the two languages actually disagree — so of
the 37 emitter goldens, not one moved.

Three things fall out of the same helper:

- **the operand of a `.`, a `[`, a call and an overloaded operator**, which is
  emitted as `left.method(right)`. A left side looser than the postfix level
  needs wrapping there for the same reason;
- **`- -a`**, which was emitted as `--a` — one token to C++'s scanner, and a
  pre-decrement of `a`. A prefix operator whose operand starts with the same
  character is wrapped;
- **parentheses the source wrote**, which are a node of their own (record
  0025's printer needs them) and were never the problem. They still come out
  exactly where they were written.

## What is not decided here

- the unary minus against `**`, above;
- `**` itself, which has no type and no emission;
- `in` and `not in` as expressions, which have neither either. Both are read
  by the parser and written back by the printer, and the compiler says so by
  name rather than emitting something that means the wrong thing.
