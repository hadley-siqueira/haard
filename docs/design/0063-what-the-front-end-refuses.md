# 0063 — What the front end refuses, so the back end never sees it

Status: **decided and built**, 2026-09-24. Hadley: *"fix each of these
problems, and make sure they do not reach the emitter"*.

| | |
|---|---|
| `break` and `continue` outside a loop are errors, and a closure's body is outside the loop it is written in | **decided** |
| An assignment, `&` and `++`/`--` need a **place**; a value is refused | **decided** |
| A `goto` names a label of its own function, a label is written once, and a jump forward may not skip a declaration still in view where it lands | **decided** |
| A local is in view from the **end** of the statement that declares it | **decided** |
| `this` outside a class is an error | **decided** |
| One scope holds one declaration of a name, except an overload set of **different** parameters | **decided**, Hadley |
| A function that promises a value gives one back on **every path** | **decided**, Hadley |

## What was there

Twenty-four wrong programs, measured on 2026-09-24. **Twenty-three** of them
passed the front end with exit status 0, and were then caught in one of three
places, each too late:

- **by g++**, about the C++: `break` at the top of a function, `3 = 4`,
  `&(a + 1)`, `goto nowhere`, `this->n` in a free function;
- **by the emitter**, in a sentence of its own with no line and no caret:
  `'y' has no type the emitter can write` for a name used before its `let`;
- **by nobody**: two classes `Box`, two fields `x` and two `f(i32)` compiled
  and ran, and an `i32` function with no `return` compiled and died at run time
  on the trap g++ writes there -- *Illegal instruction*.

To a representation between the front end and a back end, all three are the
same thing: a tree that is not a program. This record is the front end
refusing them, each where the question can be answered.

## Decision

### A loop to leave -- the StatementChecker

A count of the loops around the statement being walked, reset to none at the
start of every function and every **closure**: a closure's body is a function
of its own, so a `break` in `xs.each(|x| { ... })` written inside a `while`
has nothing to leave. That one looks right, which is why it is worth a rule.

### A place -- the ExpressionTyper

`is_place` answers whether an expression names somewhere a value is kept: a
variable, a parameter or a field; an element; what a pointer points at; or
anything whose type is a reference, which is what a call giving back a `T&`
is (record 0035). An assignment, `&` and the four increments ask it. A
literal, a sum, and a call that gives back a value are values.

### A label -- the StatementChecker

Per body, closures apart: every `goto` names a label written in that body,
and each label is written once. A jump **backwards** is always fine. A jump
**forwards** may not land where a declaration it skipped is in view -- one in
the same block as the label or in a block around it -- because that name would
be read without ever having been made, and C++ refuses it as crossing an
initialisation. A declaration in a block that ended before the label is not
in view there, and may be skipped.

Building this found that even a correct `goto` never compiled: the emitter
wrote the label as the word `label` itself.

### Order -- the UseResolver

A local -- a `let`, or an assignment that declares (record 0027) -- is in
view from the **end** of the statement that declares it. So both of these
are refused:

```haard
let y = x + 1       # 'x' is used before it is declared
let x = 2

let z = z + 1       # and so is this one
```

The resolver already finds the innermost declaration, so in a block that
redeclares an outer name, `let x = x + 1` names the inner `x` -- and is
refused. A function, a class and a global are not locals and are in view
wherever they are written, which is what `a_use_may_come_before_its_declaration`
keeps pinning.

### `this` -- the ExpressionTyper

Asked when `this` is typed: outward through the scopes, a class has to be
found. A closure inside a method finds it.

### One declaration per name -- the OverrideChecker

Record 0012 makes several functions of one name an **overload set** told
apart by their parameters. So two of anything else in one scope is an error
-- two classes, two fields, two variants, two parameters, two `let`s -- and
so are two functions with the **same** parameters, since no call could ever
choose between them. Each repeat is reported where it is written.

A name in an inner block that hides one outside is unchanged: that is two
scopes. So is a derived class declaring a method of its base, which is an
override (record 0020).

It is in the OverrideChecker because telling two functions apart takes their
signatures, and that is the phase that reads a signature and reports about a
declaration.

### Every path gives back -- the StatementChecker

Hadley's rule, not C++'s: C++ warns and lets it run. The rules are the ones a
reader checks by eye:

- a `return` or a `goto` ends a path;
- an `if` ends every path only with an `else`, and only when every branch
  does;
- a `switch` does when every case with a body does and nothing is left
  uncovered -- a `default`, or an enum, whose switch is already refused
  unless it covers every variant;
- a loop does only when it runs forever -- `while true`, or a `for` with no
  condition -- with no `break` of its own inside.

A body that is only `pass` is exempt **inside `std.low_io` and nowhere
else**: there it is a declaration whose body the emitter writes, which is what
record 0030's natives are, and the module is the same half of the match the
emitter makes. The first version exempted any `pass` body, and a user's
`def f : i32` of `pass` was measured compiling to `int32_t f() { }` and dying
on the very trap this rule closes; Hadley chose the narrow exemption the same
day. Fifteen fixtures in `tests/compilation` wrote `pass` in a non-void
function as a placeholder, and now give back a value -- their subject is
imports, and none of their goldens moved. A closure
of statements that promises a value is held to the same rule; one of a single
expression gives that expression back.

## What building it found

**A closure of one expression gave back nothing when the expression built a
String.** `xs.map(|n| { length_of("${n}") })`: records 0032 and 0061 put the
String's statements in front of the expression, and the body was no longer
one expression -- so the closure, typed to give back an `i32`, was emitted as
a function with no `return`. Undefined, and silent, since record 0032. The
rule above found it on its first run over the suites.

The fix is the rule as the source wrote it: a body is one expression when
everything in front of its last statement was put there by a pass, which is
exactly the statements whose token is synthetic. And the emitter writes those
statements before it gives the expression back.

## The tests

Each rule has a case with the wrong half and the right half side by side, so
a rule that refused too much fails as surely as one that refused too little:

- `tests/statement_checker/cases/a_break_needs_a_loop`
- `tests/statement_checker/cases/only_a_place_is_changed`
- `tests/statement_checker/cases/a_goto_needs_its_label`
- `tests/statement_checker/cases/a_function_gives_back_on_every_path`
- `tests/statement_checker/cases/this_is_written_in_a_class`
- `tests/use_resolver/cases/a_local_is_used_after_its_declaration`
- `tests/override_checker/cases/a_name_is_declared_once_per_scope`
- `tests/emitter/cases/a_goto_reaches_its_label` -- a correct `goto` that runs
- `tests/programs/cases/template_strings_are_built_when_they_run` -- the
  closure that builds a String, by what it gives back

Two older goldens moved, each by one new diagnostic that is right:
`two_methods_of_one_class_still_tie` (two `pick(i32)` in one class) and
`a_label_and_a_goto_name_nothing_here` (a `goto` to a label that is not
there).
