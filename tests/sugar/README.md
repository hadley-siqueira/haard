# SugarLowerer tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project — an `entry` naming the file to start
from, the sources, and a `table.tbl` whose `prelude` block is what puts
`String` in view (record 0033), since a case's `app/main.hd` writes no import.

## The golden is the tree, written back as Haard

`show.cpp` compiles the case and pretty-prints the entry module **after** the
lowering pass has run, so the golden is the sugar taken apart in the language
it was written in:

```
def main : i32
    let n = 7
    let __ts0 : String
    __ts0.append("n is ")
    __ts0.append(n)
    __ts0.append(" and that is that")
    let line = __ts0
    return 0
```

That is not a convenience. Record 0025 chose an **Ast → Ast pass** over an
intermediate representation with exactly this argument — the PrettyPrinter is
already an oracle for a tree, so a pass that rewrites one can be **read**
instead of inferred. This suite is that argument being cashed in.

The `std/string.hd` beside each case is the smallest String the lowering needs:
three `append` overloads, so that what the golden shows is each `${}` finding
the one its own type asks for. The real String — which owns its bytes and
writes record 0031's `copy` — is in `tests/programs/`.

## What the goldens are pinning

**That the pieces come out in order.** `a_template_string_becomes_a_string`
has a chunk, an interpolation and a chunk, and the appends are in the order
they were written.

**That an inner template string is built before the outer one appends it.**
`a_template_string_inside_another` writes `"outer ${"inner ${n}"} end"`, and
the golden has `__ts0` — the inner — complete before `__ts1` starts. The pass
walks the children of a template string before lowering it, and every
insertion goes directly before the same statement, so whatever went in first
stays in front. `two_template_strings_in_one_statement` is the same rule
sideways.

**That a chunk becomes a literal and not a copy of its token.** A chunk's token
holds the text *without* the quotes a literal has, so the literal is a token of
its own. `a_chunk_that_holds_a_quote` writes `'he said "hi" ${n} times'` — a
bare `"` is legal there and has to be escaped in the literal — and
`"one\ttwo ${n}\n"`, where an escape already written means the same thing
inside the literal and goes through untouched.

## `T[]` is a class

`an_unsized_array_is_a_class` is the second thing this pass does, and the
golden is the argument for reading a tree instead of inferring one:

```
def takes : void
    @xs : Array<i32>
    @grid : Array<Array<i32>>
    @fixed : i32[3]
```

from a source that wrote `i32[]`, `i32[][]` and `i32[3]`. Records 0016 and 0022
made `T[]` written form for `Array<T>`, and record 0021 makes a **written
length** a fixed array, which is not a class — so the third line is the one
that proves the rewrite is not simply "anything with brackets".

It hoists nothing, unlike a template string: a type is not an expression, so it
is rewritten wherever it stands — a local, a parameter, a field.

## An array literal is bound before it is used

`an_array_literal_is_bound_before_it_is_used` shows the second thing this pass
hoists. A `{}` is a C++ array and a C++ array is a **declaration**, so a
literal that is not already a binding's initialiser becomes one:

```
let bound = [1, 2, 3]
let __ar0 = [4, 5]
let __ar1 = [6, 7]
let two = total(__ar0) + total(__ar1)
```

`bound` does not move, and the hoisted ones keep the order they were written
in. `an_array_literal_cannot_always_be_hoisted` is the same three refusals a
template string gets, plus the one that is **not** refused: a global, because a
global is a binding and the emitter builds one where it is bound.

## The three refusals, and what is not refused

The calls have to become statements, and lifting them out of the expression
they were written in is not always meaning preserving. Where it is not, the
pass refuses instead of emitting a program that means something else:

- **a loop condition** — a `while`'s, or the middle of a C shaped `for`. Lifted
  out, the String is built once before the loop instead of every turn.
- **the right of `and` or of `or`** — evaluated only when the left did not
  already decide. Lifted out, it would be built even then.
- **module level** — a global's value has no statement to be built before.

`what_is_safe_is_not_refused` is the case that keeps the other three honest: an
`if` condition is evaluated once before either branch, and the **left** of a
short circuit runs whatever the right one does. Both lower in full. Without it
a sabotage that refuses every condition, or every operand of `and`, would pass
the suite.

## A refused template string is still a String

Look at the refusal goldens: the tree keeps `let __ts0 : String` with nothing
appended to it, and the node became a use of it.

That is recovery, and it is the parser's poisoned primitive one phase later. An
expression that types to nothing makes the call around it report *no `f` takes
these arguments*, so without it one mistake would read as two — the refusal,
and a consequence of the refusal. **The appends are what is dropped**, and
dropping them is what keeps the recovery from inventing errors of its own:
`for i = 0; again("${i}"); ...` would otherwise put a use of `i` before the
loop that declares it.

Module level is the one refusal with no recovery, because there is no block to
put the declaration in. It reports once as it is.
