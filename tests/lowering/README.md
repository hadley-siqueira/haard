# Lowering tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite this suite's own expected/ files
```

Or, with the other suites, from the build directory: `make check`.

Stage 4 of the IR (record 0077): the `IrLowerer` and its `FunctionLowerer`
turn a checked program into the IR. Two halves.

## This suite's own cases

`cases/` are Haard projects, one construct or a few each. The golden is the
**structured IR** the lowering wrote (what `hdc --emit-ir` prints), then what
running it in the interpreter printed and how it ended, in the words
`tests/emitter` uses: `--- output:` and `--- exit: N`. They show *what* is
generated: every slot at the top of the function, a `while` as a `loop` that
breaks when its condition fails, a `for`'s body in a `block` its `continue`
leaves so the increment still runs.

Each one's exit status was checked against the C++ path by hand when it was
written (`hdc --emit-cpp`, built and run): 78, 255, 56 and 63 for step 1;
15, 15, 15, 15 and 7 for step 2's five that end in one. Step 3's four:
7, 63, 15 and 31, each counting with a global the order `init`s and
`destroy`s ran in, since printing needs `String`, a class. Step 2's other three
show what only the IR says, since the C++ is undefined there or does not
check yet: an index past a fixed array aborts (question 9b), a read after a
`delete` and a second `delete` stop the interpreter.

## Every program of tests/emitter

Then every project of `tests/emitter` is lowered, flattened and run, and must
end the way the C++ it was compiled to did: the same output, the same exit
status, read off the end of its golden there. That suite's goldens already
held the answer; this is the second path to it that record 0077 wanted, and
the oracle it called the interpreter.

A program the lowering cannot do yet stops with `not lowered yet: ...` naming
what it met, and is written in `not_yet`. Those are counted apart. A program
still on the list that lowers now is a **failure** until it is taken off, so
the list never says less than is true; and a program that lowers and ends
otherwise than the C++ is a failure whether it is listed or not.

The list shrinks step by step, in the order record 0077 gives: (1)
functions, locals, arithmetic, control flow, calls -- done; (2) pointers,
references, fixed arrays, globals, literals, `new`/`delete` -- done; (3) structs,
methods, `init`/`destroy`, the cleanup stack, temporaries -- done; (4) classes,
vtables, `vcall`, `super`; (5) enums and `switch`, tuples, unions, closures,
symbols; (6) the standard library.

## What step 1 found

`control_flow_and_operators` was the first program of the emitter's suite to
run through the IR, and its `classify` -- an `if` with two `elif`s and an
`else`, every one returning -- broke flatten: the block after the inner `if`
was dead and empty, and the jump flatten wrote out of it kept the block after
the outer one alive, empty, and the verifier refused it. Fixed in flatten, and
pinned in `tests/ir` by `run_an_if_chain_whose_every_side_returns`.

## What step 2 found

Three things on the C++ path, none in the IR, while writing its cases: a
braced literal of braced literals (`{{1, 2, 3}, {4, 5, 6}}`) and a fixed array
copied by `let b = a` pass the front end and come out as C++ g++ refuses; and
a global whose value reads a global declared below it gets no type. The
cases were written around them; record 0077 lists them.

## What step 3 found

Its own bug, before it was committed: a temporary made on the right of an
`and` was ended whether or not that side ran, since the depth that says an
expression is on one side only was never raised. And two sabotages first
passed because they were too weak, not the cases: reversing the fields'
order changed only their types, both `T`; and dropping the depth's `++`
left its `--` to wrap round, after which every temporary had a flag.

## Sabotage

Each rule of step 1 was broken on purpose, one at a time, and the suite run.
A restored file is touched afterwards, or the shared object cache keeps the
sabotaged object.

| Broken | Fails |
|---|---|
| a `for`'s `continue` going back to the top, skipping the increment | `control_flow_becomes_regions` |
| an `elif` read as an `else` | that, and `emitter/control_flow_and_operators` |
| `and` and `or` running both sides | three cases |
| `/` ignoring the sign | two cases |
| `//` truncating instead of flooring | `whole_numbers_keep_haards_rules` |
| `>>` on a signed number shifting in zeroes | `whole_numbers_keep_haards_rules` |
| a widening ignoring the sign | `whole_numbers_keep_haards_rules` |
| `i++` giving the new value | `increments_and_powers` |
| a negative power of 1 giving 0 | `increments_and_powers` |
| `**` multiplying once too often | `whole_numbers_keep_haards_rules` |

`i++` first passed: no case used the value of one. And a negative power was
only caught by the golden's text, since no case raised to one; both were
written into `increments_and_powers`, checked against the C++ path, and the
two run again.

And each of step 2's, the same way:

| Broken | Fails |
|---|---|
| `check_index` never aborting | `an_index_past_a_fixed_array_aborts`, and `tests/ir` |
| `check_index` reading the index as signed | `tests/ir`'s `run_check_index_aborts_past_the_length` |
| no `check_index` written | three cases |
| a count always widened by `zext` | six cases |
| `p - n` moving on | `pointers_move_and_compare`, `emitter/a_pointer_moves_by_a_whole_number` |
| `q - p` in bytes | the same two |
| a `T&` parameter given a copy | five cases |
| a reference local not read through | `a_reference_is_an_address`, `emitter/an_inclusive_range_reaches_the_top_of_its_type` |
| `new T` not zeroed | `the_heap_is_two_natives`, and one more |
| `__free` leaving the allocation live | the two that free, and `tests/ir` |
| the initialisers not run | `globals_are_given_their_values_before_main`, and `tests/ir` |
| `\$` kept as written | that, and `emitter/a_dollar_sign_is_escaped` |
| `argc` of 0 | `emitter/main_takes_argc_and_argv` |
| `sizeof` measuring a pointer | `emitter/sizeof_measures_a_type` |
| a braced literal written one element on | five cases |
| `p--` moving on | `pointers_move_and_compare` |

And step 3's:

| Broken | Fails |
|---|---|
| a scope's locals ended first to last | two cases |
| a `break` ending nothing | `a_struct_ends_with_its_scope` |
| a `return` ending nothing | two cases |
| a temporary kept to the end of its block | two cases |
| a struct given by value not copied | two cases |
| no named return value | three cases |
| the fields ended first to last | two cases |
| a field's written value not given | `a_struct_is_built_where_it_goes` |
| an assignment without its self check | `copies_moves_and_assignments` |
| an assignment that does not destroy first | the same |
| `= &&x` copying instead of moving | the same |
| a struct assigned by its bytes | the same |
| a struct copied by its bytes | two cases |
| `new T` not zeroed | `operators_and_the_heap_of_structs` |
| `delete` not ending the struct | the same |
| `delete` of null ending it | the same |
| no flag for a temporary on one side | the same |
| `super(...)` not run | `a_struct_is_built_where_it_goes` |
| a result built in a temporary instead of where the caller said | three cases |
| the `destroy` method never run | three cases |
