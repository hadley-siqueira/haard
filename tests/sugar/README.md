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
in. `an_array_literal_is_built_where_it_runs` is the same rewrites a template
string gets (below), plus the one place that needs none: a global, because a
global is a binding and the emitter builds one where it is bound.

## Where building before the statement would change the program

The calls have to become statements, and lifting them out of the expression
they were written in is not always meaning preserving. Record 0032 refused
three places; record 0061 **rewrites** four into the control flow they already
are, and the goldens are that rewrite, readable:

- **`a_short_circuit_becomes_a_branch`** — the right of `and` or `or` runs only
  when the left did not decide. The operator becomes a `bool` flag and an
  `if` that sets it, and the right is built inside that `if`. `nested` is a
  chain: the `and` on the left of the `or` builds nothing and stays an
  operator, and the `and` on its right is taken apart again inside the branch.
- **`a_loop_condition_is_built_every_turn`** — a `while`'s condition, and a C
  shaped `for`'s, move to the top of the body as `if not c: break`, so they are
  built every turn. `steps` is a `for` whose **step** builds something: it
  moves to the top behind a flag the header's step sets, since at the end a
  `continue` would skip it. Its last loop holds a **closure** in the
  condition, whose body builds in a block of its own, so that loop is left as
  written.
- **`an_elif_becomes_an_else`** — an `elif`'s condition runs only when the
  branches above were false. It used to be built before the whole `if`, and
  nothing refused it. The `elif` becomes the `else` holding an `if` it always
  meant, and what follows it moves inside.
- **`an_array_literal_is_built_where_it_runs`** — the same rewrites for a
  bracket literal that is not bound, plus the one place that never needed
  one: a global, which the emitter builds where it is bound.

**Module level is the one refusal left**,
`at_module_level_there_is_no_statement`, because a global's value has no
statement to be built before. Record 0032 turned a refused template string into
an empty `String` so one refusal did not read as two errors; module level never
had a block to recover into, so that recovery went with the other refusals.

`what_is_safe_is_not_rewritten` is the case that keeps the others honest: an
`if` condition is evaluated once before either branch, and the **left** of a
short circuit runs whatever the right one does. Both lower as they always did,
with no flag and no branch. Without it a sabotage that rewrites every
condition, or every operand of `and`, would pass the suite.

**What these goldens do not say is whether the rewrite is right.** They pin
the shape. The meaning is pinned by
`tests/programs/cases/template_strings_are_built_when_they_run`, which logs
when each `${}` runs and reads no C++, so it stays true under any lowering.

## The sabotages of record 0061

| sabotage | fails here | and in `tests/programs` |
|---|---|---|
| a short circuit is never rewritten | 2 | yes |
| a loop condition is never moved into the body | 2 | yes |
| a `for` is never taken apart | 1 | yes |
| a `for`'s step never moves, only its condition | 1 | yes |
| an `elif` is never made an `else` | 1 | yes |
| `hoists` looks inside a closure's block | 1 | no — the shape changes, the meaning does not |
| `or` is tested like `and` | 2 | yes |
