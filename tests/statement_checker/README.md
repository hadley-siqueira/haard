# StatementChecker tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project: an `entry` file naming where to start
and the sources it reaches. `check.cpp` compiles it and prints the
diagnostics, so the golden is **what was reported and nothing else** — which
is the only thing this phase produces. A case whose golden is `no diagnostics`
is a case that says a correct program stays quiet, and half the lines in every
file here exist for that.

## What each case pins

| case | what it proves |
|---|---|
| `a_return_is_checked_against_its_function` | the return type is handed down, so `return 200` in a `u8` is a u8 literal and `return 300` is about the value; a name and a call are compared by equality; and a `return` nested in an `if` inside a `while` is reached |
| `a_return_that_gives_nothing_or_too_much` | the two halves of `void`: a bare `return` in a function that promised something, and a value in one that promised nothing |
| `a_condition_must_be_bool` | `if`, `elif`, `while` and the middle of a C shaped `for`; that `for ; ; i++` is a loop and not a mistake; and that a comparison already is a `bool` |
| `an_assignment_wants_one_type_on_both_sides` | the left side decides and the right is asked to be it; a compound form asks the same question; and `n = n = 2.5` reports once |
| `a_binding_is_not_checked_twice` | the golden is **two** errors and not four. A binding's initialiser belongs to the `TypeCollector`, and this phase walks through it without typing it |
| `a_signature_that_could_not_be_built_asks_nothing` | a `def` whose type names nothing is reported where the name is written, once, and its `return` is not compared against a signature that does not exist |
| `this_and_a_method_return` | `return this` in a method, which until now was typed by nothing at all: the typer only ever ran on a binding's initialiser |
| `a_condition_written_with_and_or_not` | the logical operators, in both spellings: `n and n` reports **twice**, because the second operand is asked even after the first was wrong, and `1 and flag` is a complaint about the literal and not about the operator |
| `a_closure_gives_back_nothing_known` | a `return` inside a closure is compared against nothing, an assignment inside one is still checked, and a `return` **after** the closure is still the function's — which is what makes the return type a parameter of the walk and not a member |

## The one that is not obvious

`a_signature_that_could_not_be_built_asks_nothing` has a second function whose
**parameter** names nothing while its return type is a perfectly good `i32`,
and its `return 2.5` is **not** reported. That is record 0016's poison rule
reaching further than it looks: a part that cannot be built poisons what
contains it, so one bad parameter leaves the whole signature `INVALID_TYPE`
and there is no return type left to check against.

It is the right silence — one mistake gets one message — but it is a silence,
so the case exists to make it visible. Removing the `INVALID_TYPE` guard in
`check_return` turns it into a second, confusing complaint about a literal.

## Every rule here fails a case when it is taken away

Thirteen sabotages, one per rule, and each of them breaks at least one golden.
A control edit that changes no behaviour leaves all 8 passing, which is what
says the suite is measuring the rules and not the shape of the file.

| what was taken away | cases that failed |
|---|---|
| the guard against an unbuilt signature | 2 |
| handing the return type down to the literal | 2 |
| the bare `return` in a function that promised something | 1 |
| the value in a `void` function | 1 |
| `elif` among the conditions | 1 |
| the middle part of a C shaped `for` | 1 |
| handing `bool` down to the literal in a condition | 1 |
| the compound assignment forms | 1 |
| the left side deciding what the right side is asked to be | 2 |
| the return being the **last** argument of the signature | 1 |
| the closure resetting what is being returned | 1 |
| checking a `return` at all | 4 |
| checking an assignment at all | 2 |

The `ExpressionTyper`'s own rules for `and`, `or` and `not` are sabotaged
against this suite **and** `tests/type_table/`, since the type they give back
is only visible there: eight more, each breaking at least one golden of one of
the two.

## What is not checked, and why

**What a closure gives back.** A closure is not a declaration, so it has no
candidate and no signature, and a `return` inside one is compared against
nothing.

**The first and third parts of a C shaped `for`.** `for i = 0; ...` is an
assignment and is checked as one, wherever it is written — but nothing says
those parts must be assignments at all.
