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
| `the_coercion_list_is_not_only_for_a_call` | record 0023: the return, the binding and the assignment ask record 0018's list, which until 2026-09-03 only a call knew. Six forms pass in silence and nine are refused — an upcast by value, a base where a derived was asked, a numeric widening, and a class with `String`'s own fields under another name, which is what pins that `char*` reaches `String` because the language names it and not because a class holds a `char*` |
| `the_increment_of_a_for_is_checked` | the last part of `for a; b; c:` was typed by nothing until 2026-09-03, so a wrong call passed there in silence while the same line inside the body was reported. Found by the emitter refusing to name something the type phase had never looked at. The golden is **three** errors and not four: the head is not checked here on purpose, because an assignment there declares and its initialiser belongs to the `TypeCollector` |
| `a_call_written_as_a_statement` | a call whose answer is thrown away is type checked at all, which nothing did until 2026-09-02 |
| `a_closure_gives_back_nothing_known` | written before closures typed, and it still holds with a new reading since record 0058: a `return` inside a closure answers to the **closure's** `-> f64`, not to `main`'s `i32`, an assignment inside one is still checked, and a `return` **after** the closure is still the function's — which is what makes the return type a parameter of the walk and not a member |
| `a_closure_answers_to_its_own_return` | record 0058: a `return` inside a closure is checked against what the closure wrote, what was expected where it stands, or `void` when neither says; a one-expression body is checked against the `bool` its call asks for, and **once** — typed again as a statement of the body, the `'and' needs bool` inside it would come out twice; a closure that could not be typed is not walked, so its body's use of the untyped `z` says nothing more |
| `a_condition_reads_through_a_reference` | record 0035 reaching the two places that ask for a `bool`: a `bool&` is a bool in an `if`, a `while` and an operand of `and`, and was refused as `bool&` in all three until 2026-09-22. The golden is one error, the `i32&`, where the compiler before it gave three |
| `a_closure_is_typed_once` | record 0040's lowering types a `for ... in`'s sequence twice — once to learn what it is, once as the `iterator()` call it becomes — and a closure inside it is typed once all the same: the mistake in its body is said once |
| `a_function_value_is_called_by_its_type` | a value of type `A -> R` is called by its type and nothing is chosen: the wrong count, a literal that cannot be the parameter, and a name that is no function are each said in words about the call. And a closure given where one is expected must agree with it wherever it wrote something itself — how many it takes, a parameter's type, what it gives back |

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

## Record 0058's sabotages, across four suites

A closure's rules live in the typer, the collector, the resolver, this checker
and the emitter, and are pinned by cases in `type_table`, here, `emitter` and
`programs`. Each of the 29 was taken away on 2026-09-22 and each broke at
least one case; four broke none the first time, and the case that now catches
each was written for it (marked).

| what was taken away | cases that failed |
|---|---|
| a parameter's type taken from the `A -> R` expected | 5 |
| the sweep skipping a closure's body | 1 |
| the body's locals typed with the closure | 1 |
| a closure typed once (`a_closure_is_typed_once`, written for it) | 1 |
| the expression given back checked once | 1 |
| a `return` answering to the closure | 1 |
| a closure that never typed not walked | 1 |
| the expression given back told what to be (`wide`, written for it) | 1 |
| a mismatch in what is given back reported | 1 |
| what is given back deciding the type when nothing else does | 2 |
| nothing written and nothing expected is an error | 2 |
| how many it takes, against what is expected | 1 |
| a written parameter type, against what is expected | 1 |
| a written `->`, against what is expected | 1 |
| a variable, parameter or field called as a value | 4 |
| the count of a call through a value | 1 |
| an argument of such a call asked to fit (`ratio`, written for it) | 1 |
| a literal argument of such a call told what to be | 1 |
| a callee that is no name called as a value (`make()(4)`, written for it) | 1 |
| a closure argument carried untyped until an overload wins | 4 |
| a closure argument typed against the winner | 4 |
| an overload chosen by how many a closure takes | 1 |
| `(A, B) -> C` flattened into two parameters | 2 |
| a capture bound by reference inside the closure | 2 |
| `this` written as the captured pointer inside the closure | 2 |
| the environment filled in where the closure is written | 2 |
| a one-expression body emitted as a `return` | 1 |
| a `def` as a value written through its adapter | 1 |
| a call through a value written through its helper | 2 |

## What is not checked, and why

**What a closure gives back** is checked since 2026-09-22 (record 0058). A
closure is still not a declaration and has no candidate, so what it gives back
is read off the type the typer recorded on the closure node.

**The first and third parts of a C shaped `for`.** `for i = 0; ...` is an
assignment and is checked as one, wherever it is written — but nothing says
those parts must be assignments at all.

## What record 0063 pins

Each case holds the wrong half and the right half side by side, so a rule
that refused too much fails as surely as one that refused too little.

- **`a_break_needs_a_loop`** -- `break` and `continue` with no loop around
  them, and the one that looks right: a `break` in a closure written inside a
  `while`, which is a function of its own. A loop inside a closure is fine.
- **`only_a_place_is_changed`** -- an assignment, `&` and `++` of a value:
  `3 = 4`, `five() = 6`, `a + b += 1`, `&(a + 1)`, `++5`. And every place:
  a variable, a parenthesis around one, a field, an element, a call that
  gives back a `T&`, what a pointer points at.
- **`a_goto_needs_its_label`** -- a label that is not there, one in another
  function, one written twice, and a jump forward over a declaration in view.
  A jump backwards, and one forwards over a block that has ended, stand.
- **`a_function_gives_back_on_every_path`** -- five functions and a closure
  that can reach their end, and nine that cannot: both branches, every
  variant, a `default`, `while true`, `for ; ; :`, a `break` of an inner loop,
  a `pass` body, and a closure of one expression.
- **`this_is_written_in_a_class`** -- `this` in a free function, twice, and
  in a closure inside a method, where it is fine.
