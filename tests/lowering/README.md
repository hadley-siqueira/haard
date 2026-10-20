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
written (`hdc --emit-cpp`, built and run): 78, 255, 56 and 63.

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
references, fixed arrays, globals, literals, `new`/`delete`; (3) structs,
methods, `init`/`destroy`, the cleanup stack, temporaries; (4) classes,
vtables, `vcall`, `super`; (5) enums and `switch`, tuples, unions, closures,
symbols; (6) the standard library.

## What step 1 found

`control_flow_and_operators` was the first program of the emitter's suite to
run through the IR, and its `classify` -- an `if` with two `elif`s and an
`else`, every one returning -- broke flatten: the block after the inner `if`
was dead and empty, and the jump flatten wrote out of it kept the block after
the outer one alive, empty, and the verifier refused it. Fixed in flatten, and
pinned in `tests/ir` by `run_an_if_chain_whose_every_side_returns`.

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
