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
| `**`, `//`, `>>>` and `in` had no back end, and two of them no type | **built** |

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

`**` is typed and emitted since the same day, below, so this is visible in a
program that runs. The parser suite dumps the tree, which is where the
associativity itself is proven.

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

## The four the table had and the compiler did not

Four operators of the table were precedence with nothing behind them, and they
had failed in **two different ways**, which is worth keeping apart:

- `**`, `in` and `not in` were **not typed**. `ExpressionTyper::work` had no
  case for them, so they fell through its `default` and gave back
  `INVALID_TYPE` *without a word* — and the first complaint was three phases
  later and about the name the value had been bound to. `let a = 2 ** 3` said
  *'a' has no type the emitter can write*, which is a true sentence about the
  wrong line;
- `//` and `>>>` **were** typed and were not **emitted**, one step further
  along, and the emitter said so where it happened.

All four run now.

**`**`** types exactly as `*` does: both sides the same type, and the answer
is that type. C++ has no operator for it, so what comes out is a call to a
helper the emitter writes above the program — repeated multiplication for a
whole number, `std::pow` for a float. A negative exponent over a whole number
is `1 / (a ** -b)` truncated toward zero, which is `0` for every base but `1`
and `-1`: the honest answer, rather than a special case that silently means
something else. A class may overload it, like every other arithmetic operator.

**`//` floors**, and that is a decision and not an implementation detail:
C++'s `/` truncates toward zero, and the two differ exactly when the operands
have different signs and the division is not exact. Emitting `/` would have
been emitting a **different operator**, silently, for negative operands only.
So it is a helper too — except over an unsigned pair, where C++'s `/` already
floors and is used as it is.

**`>>>`** shifts through an unsigned of the same width and casts back, which
is what filling with zeroes means. Over a type that is already unsigned it *is*
C++'s `>>`, and is emitted as it.

**`a in b` is `b.contains(a)`.** The container is asked for a method by name,
which is how this language asks a type for anything — record 0040 asks one for
`iterator`, `has_next` and `next`, and record 0051 asks a list for `add`.
There are no interfaces. The name is `contains` because `Range<T>` already had
one doing exactly this before anything called it.

Two things fall out of it:

- **the compiler writes the type argument**, out of the value's type. `in` has
  nowhere to put a `<i32>`, so a call that could not be *written* is one the
  typer *builds* — which is also why none of this waits on inference for a
  generic called with no type arguments, measured on 2026-09-10 and still
  missing;
- **a generic container's `contains` is a generic METHOD**, taking a `U` of
  its own rather than the class's `T`. A method taking `T` is instantiated
  **with the class** (record 0054), so comparing two `T` there would make
  every `T` ever put in an `Array` need an `operator==` for a method nobody
  called — measured again here, and it is still exactly what stops
  `Array<Token>` from compiling. A generic method is instantiated per call
  (record 0055) and one nobody calls is never instantiated at all.

`Hash<K, V>`'s is **not** generic: a hash cannot work without comparing keys,
so every `K` already needs an `operator==` and there is no trap to avoid.
`String`'s is not either — it holds bytes and there is nothing to bind.

The value is typed **expecting what the container holds** — the first argument
of its instantiation, when it has one — so `7 in longs` makes the `7` an `i64`
instead of letting it take its default and then failing to compare.

## `[T]` and `{K: V}` were never lowered

Found while writing the section above and fixed the same day.

Record 0022 makes `T[]`, `[T]` and `{K: V}` written form for three classes of
the standard library. Only the **first** of them was ever rewritten. The other
two built a structural type of their own — `TYPE_LIST` and `TYPE_HASH` — so
this compiled and then answered to nothing:

```haard
let l : [i32]
l.push_back(2)        # [i32] has no member named 'push_back'
let h : {i32: i32}
h.length()            # {i32: i32} has no member named 'length'
```

while the same program written `List<i32>` and `Hash<i32, i32>` worked. The
sugar reached the **literal** and not the **annotation**, which is what makes
this a hole and not a decision: nothing anywhere had decided that `[T]` should
be a second thing.

One rewrite serves all three now — `SugarLowerer::lower_into_generic`, whose
whole difference between them is that a hash has two children and the other
two have one. `T[3]` is untouched: record 0021 makes a written length a fixed
array, which is not a class, and the length is what tells the two apart.

**`TYPE_LIST` and `TYPE_HASH` are gone with them.** Nothing built one after
this, and nothing had ever *emitted* one — which is what they had been all
along, a shape with no back end. That took four sites in the type phase with
them, and two test dumpers that keep their own copy of the rendering.

Three cases in `tests/type_table` had been writing those spellings as shapes,
and they say something different now. Two of them lost a line each and gained
a sentence saying why; one needed a `Hash<K, V>` declared in the file, since a
name has to be in view for the same reason `Array` does; and one had its
refusal checked against a tuple instead, because a type the file cannot name
says nothing about what it refuses.

## A cast read no reference

Found the same day, in a sample in the README's own tour that did not compile,
and fixed.

Record 0049's list was asked about the **written** types only. So `symbol as
char*` was on it and `symbol& as char*` was not — and the loop variable of a
`for x in` is a reference (record 0040), which made this the diagnostic:

```
error: there is no cast from symbol& to char*
    println("${who as char*} is ${ages[who]}")
```

a sentence about a `&` the reader never wrote. Record 0035 already says what
the answer is: a reference **is** the thing it names, so it casts wherever the
thing does. `may_cast` reads through one now and asks itself the same list one
step in.

It **joins** the base chain and does not replace it. That entry is about a
reference on purpose — a base is reached through a pointer or a reference —
and reading through both first leaves `Circle as Shape`, which is the slicing
it refuses. Measured by asking only the new one: `circle_ref as Shape&` and
`shape_ref as Circle&` stop casting.

The order between the two is **not** load-bearing, and the first version of
this section said it was. Swapping them changes no answer, because a class by
value is refused either way — measured by swapping them. What the case pins is
the answers, not the order: `solid as Shape&` is still refused, which is the
slice arriving from the other side.

## What is not decided here

- the unary minus against `**`, above.
