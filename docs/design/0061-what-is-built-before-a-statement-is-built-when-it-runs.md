# 0061 — What is built before a statement is built when it runs

Status: **decided and built**, 2026-09-24. Hadley asked for record 0032's
three refusals to be lifted; how each is lifted is record 0032's own
sentence, and building it found a fourth place that had never been refused.

| | |
|---|---|
| A template string or an unbound literal on the right of **`and`** or **`or`** is built only when that side runs | **decided**, Hadley; the shape is record 0032's |
| One in a **loop condition** is built every turn, and one in a C shaped `for`'s **step** after every turn, `continue` included | **decided**, Hadley; the shape of the step was decided while building it |
| One in an **`elif`** condition is built only when that condition is asked | **found while building it**, a silent defect and not a refusal |
| **Module level** stays refused | unchanged: there is no statement to build before |

## What was there

Record 0032 lowers a template string into a local `String` and a run of
`append` calls **inserted before the statement it was written in**, and record
0037 hoists a bracket literal the same way. That is only the same program when
the statement evaluates the part exactly once, and before anything that could
decide not to. Three places were known not to, and refused:

```
error: a template string is built before the statement it is written in, so it cannot go on the right of 'and'
error: a template string is built before the statement it is written in, so it cannot go in a loop condition
```

## Decision

Each of those places is rewritten **first** into the shape it already means,
one where the part sits at the top of a statement of its own, and the hoisting
is then the same hoisting it always was. The pass does not learn a second way
to build a template string; it learns four rewrites of control flow, and each
one happens only when what it holds would be hoisted. A program with no
template string or unbound literal in any of these places gets exactly the
tree it always did, so not one emitter golden moved.

### The right of `and` and `or` is a branch

```haard
if ok and check("only sometimes ${n}"):      let __sc0 : bool = ok
    ...                                      if __sc0:
                                                 let __ts1 : String
                                                 __ts1.append("only sometimes ")
                                                 __ts1.append(n)
                                                 __sc0 = check(__ts1)
                                             if __sc0:
                                                 ...
```

`or` is the same with `if not __sc0`. The flag's type is written, `bool`, and
not inferred, so a left that is a `bool&` is read through (record 0035). A
chain takes itself apart: the new `let` and `if` are walked as statements, so
an `and` on the right is rewritten again inside the branch, and an `and` on
the left that builds nothing stays an operator.

### A loop condition is asked at the top of every turn

```haard
while again("every turn ${n}"):              while true:
    ...                                          let __ts0 : String
                                                 ...
                                                 if not again(__ts0):
                                                     break
                                                 ...
```

A C shaped `for` whose **condition** builds something empties the condition
and does the same. Its step stays where it is, and a `continue` still reaches
the step and then the condition, which is what C's `for` promises.

### A step moves to the top, behind a flag

A step that builds something cannot move to the **end** of the body: a
`continue` would skip it. It moves to the **top**, where it runs on every turn
but the first, and the condition — which runs after the step — moves with it:

```haard
for i = 0; i < 3; i = i + step("${i}"):     let __step0 : bool = false
    ...                                     for i = 0; ; __step0 = true:
                                                if __step0:
                                                    let __ts1 : String
                                                    ...
                                                    i = i + step(__ts1)
                                                if not i < 3:
                                                    break
                                                ...
```

The flag is declared before the `for`, so it starts false every time the loop
is **reached**, and a loop inside another loop gets a fresh one each time.

### An `elif` is the `else` it always meant

```haard
if a:                                        if a:
    ...                                          ...
elif check("${n}"):                          else:
    ...                                          let __ts0 : String
else:                                            ...
    ...                                          if check(__ts0):
                                                     ...
                                                 else:
                                                     ...
```

Only the first `elif` that builds something moves. Every `elif` and `else`
after it moves into the new `if`, and the walk reaches them there.

## What building it found

**An `elif`'s condition was built before the whole `if`.** Nothing refused it:
the walk treated an `elif`'s condition as part of the `if` statement, so
`if n > 0: ... elif check("${n}"):` built the String **before `n > 0` was
asked**. It was built even when the first branch ran, and it read values from
before the `if` decided anything. That compiled in silence from record 0032 on,
and a `${}` that calls something with a side effect ran it on every pass
through the `if`.

## What changed besides

**The recovery is gone.** Record 0032 turned a refused template string into an
empty `String`, so that one refusal did not read as two errors. The only
refusal left is module level, and it never recovered, because there was no
block to recover into. `SugarLowerer::recover` is deleted and nothing replaces
it.

**Order within one statement is unchanged, and not decided.** A hoisted part
was already evaluated before everything else in its statement: in
`f(g(), "${h()}")`, `h` runs before `g`. Rewriting a short circuit adds one
case of that: `x = g() + (a and f("${y}"))` evaluates `a` before `g`. C++
leaves the order of a call's arguments unspecified, and Haard has never said
what its order is. If it ever says left to right, the fix is the same
rewrite, applied to everything left of the part as well.

## What was rejected

- **A chain of `append`s as one expression**, `String().append(a).append(b)`.
  Record 0032 rejected it because it needed an unnamed stack temporary, and
  record 0045 has one since. It is still rejected: `append` would have to give
  back a `String&`, which changes the library to suit the sugar, and an
  unbound **literal** would still need hoisting. The rewrites handle both.
- **A step at the end of the body.** A `continue` skips it.
- **Refusing an `elif`** like the others. It was never a question of
  refusing: the rewrite is two lines of the same kind as the rest.

## The tests, and why they outlive the back end

Two suites pin this, and **neither reads C++**:

- `tests/sugar/` prints the tree back **as Haard** after the pass. Its cases
  show each rewrite in the language it was written in:
  `a_short_circuit_becomes_a_branch`,
  `a_loop_condition_is_built_every_turn`, `an_elif_becomes_an_else` and
  `an_array_literal_is_built_where_it_runs`. They pin the **shape**.
- `tests/programs/cases/template_strings_are_built_when_they_run` pins the
  **meaning**. Every `${}` calls `Log.seen`, which writes into a log, and each
  check compares the log. So it says **when** each part ran and nothing about
  how, and it passes unchanged under any lowering that keeps the meaning,
  including the intermediate representation record 0025 expects. When that
  representation replaces this pass, the sugar goldens are the part to
  rewrite, and this program is the part that says whether the rewrite is
  right.
