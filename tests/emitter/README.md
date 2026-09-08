# Emitter tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout` — the same four
every other suite needs, and `g++` twice over: once for the compiler's own
sources and once for the C++ the compiler wrote.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project, the same shape `tests/compilation`
uses:

```
cases/<name>/
    table.tbl        the roots table, or neither
    entry            one line: the file to start from
    <the sources>    real Haard, run through the whole front end
```

## The last line of the golden is the suite

`run.sh` takes the C++ that came out, **compiles it with `g++` and runs it**,
and writes the exit status at the end of the golden:

```
--- exit: 145
```

That line is the point. The generated text is kept in the golden so a change to
it is visible and reviewable, but every tree over the same program can be
printed many ways and only one thing says the transpiler is right: the binary
runs and gives the right answer. A case whose output `g++` refuses gets
`--- did not compile` and the errors, which is the failure this suite exists to
catch.

A case the emitter **refused** has no program to build, and its golden is the
sentence it gave instead. That is a behaviour and not an absence: a kind the
emitter does not claim has to reach the reader as words, never as C++ that
compiles and does something else.

## What each case pins

| case | what it proves |
|---|---|
| `classes_inheritance_and_overriding` | 145, and every part of it is a decision. `Square.describe` overrides `Shape.describe` and is reached three ways — through a **base pointer**, through a **bare name inside an inherited method** (record 0020), and through a **`.` on a pointer** (record 0018's third coercion, which C++ does not have). `pick` is overloaded and the two calls choose different ones. Naming a method by its candidate, dropping `virtual`, or writing `.` where `->` was needed each move the number |
| `control_flow_and_operators` | 147: `while`, a C shaped `for`, `if`/`elif`/`else`, arithmetic, comparison, the bitwise four, `not`/`and` in their word spellings, and a declaring assignment. A wrong precedence or a loop that runs the wrong number of times moves it |
| `two_modules_that_declare_one_name` | 49. Two modules each declare `helper` and they are different code, which is what record 0010 puts in one program when two versions of one library are reached. Each is called from inside its own module, so no call is ambiguous. Emitting source names makes them **one** function in the C++ and the linker picks a body — the answer is then 40 or 90, so the exit status tells the two apart |
| `a_program_of_many_modules` | 79, from **four modules in one C++ file** — which is what a real program looks like. A class deriving from one in **another module** and overriding its method, a field of that base reached by a **bare name across the boundary** (which did not work until 2026-09-03), an **aliased** import and a **qualified** call, a field initialiser, an `init` with a defaulted parameter, and a **destructor** observed by writing through a pointer it was handed. The emitter orders `Shape` before `Box` although Box's module was found first |
| `a_class_that_cannot_be_built_says_so` | every `init` of a class needs an argument, so a value of it cannot come into being and the emitter refuses by name — rather than emitting C++ that `g++` refuses in words about a line nobody wrote |
| `what_cannot_be_emitted_says_so` | a hash literal has no meaning yet (agenda 1.23), and the emitter says so by name instead of writing something |

## Why the goldens are not sensitive to the mangled names

They are, and that is accepted. A name is `h<module>_<candidate>_<source>`, so
inserting a declaration renumbers everything after it and the goldens churn.
The alternative — hashing the name, or numbering it by something stable — buys
less than it costs: the indices are what make the name unique without a
lookup, and a golden that churns is a golden somebody reads.

## What the 2026-09-06 cases pin

**`an_operator_is_a_method`** — record 0034, and the point of it is what is
*not* in the golden. Search the emitted C++ for `operator` and every hit is
`m_operator_*`, a method name: Hadley's rule is that the emitter must not lean
on C++'s own operator overloading, so `a[0]` is a call and `a[0] = 5` is a
plain assignment through the reference that call gives back. The class declares
`operator+` **and** `operator-` on purpose — folding every punctuation
character to `_` makes those one C++ name, and the case fails when it does.

**`a_reference_is_the_thing_it_names`** — record 0035, and all six places a
`T&` has to read as a value: a binding, arithmetic, a call, a comparison, an
assignment out of one, and writing through one. Before it, none of them worked,
so giving a reference back from a function was pointless.

**`a_generic_names_itself`** — record 0036. `Pair<T>` written inside `Pair<T>`,
which record 0031 makes every container that owns memory do. It used to be
instantiated with an unbound `T`, cloning a class whose fields are type
parameters. It also catches the second hole of that record: a class with `copy`
and no `init` loses C++'s implicit default constructor, so it compiles in Haard
and fails in **C++**.

## What the record 0037 cases pin

**`a_fixed_array_is_a_cpp_array`** — the primitive. A `{}` is a C++ array and
nothing else, and it could not be emitted at all until 2026-09-06. The second
half is the decay: `i32[3]` where an `i32*` was asked for is on record 0018's
list now, and without it a `{}` could never reach the `init(T*, i32)` it is
written for.

**`a_bracket_literal_is_an_array`** — the `Array<T>` is declared **in the same
file**, which is the point: the compiler knows the *name* and resolves it in
the scope the literal was written in, so a program that declares its own gets
its own. It covers all four placements — a constant literal (file-scope
`static`), a computed one (a local), one that is not bound at all (hoisted),
and one at module level (a global, which is what catches the statics being
spliced one line too late).

## What the record 0023 case pins, since it grew a second class

**`a_char_pointer_reaching_a_string`** was four examples of Hadley's about the
one class the compiler knows by name. On 2026-09-08 it grew a `Note`, which is
the same mechanism for a class the compiler has never heard of (record 0037),
and it holds two things nothing else does:

- `return h0_1_String("returned")`. The return was the one place the emitted
  C++ handed a `char*` to something that promised a class and let **C++** find
  the conversion. It is the only line of the golden that changed when the
  mechanism moved, which is what says the other three were already writing a
  construction — with C++ choosing *which* constructor.
- `copied.m_assign(const_cast<Note&>(static_cast<const Note&>(Note("copied!"))))`.
  Assigning a literal to a class that has **no** `operator=` goes through
  record 0031's copy assignment, which takes a reference. The emitter used to
  hand it the raw `char*`, and **g++ refused it** — a bug shipped since record
  0031, invisible because both classes in the suites that are assigned a
  literal declare `operator=`. This suite compiles what it emits, which is why
  the case catches it.

## What the record 0041 case pins

**`a_symbol_is_an_entry_in_a_table`** — the table itself, and the two things
that follow from it. `:status` written three times is `__symbols[0]` three
times, including once as `:'status'`, so `same(a, b)` compiles to `a == b` on
two pointers; and `s as char*` reads the name back out of the entry, which the
case counts to get its exit status. The table is `const` twice over and is
spliced above everything that can name one.

## What the record 0040 case pins

**`a_foreach_is_a_loop_over_what_it_walks`** — all three shapes of `for x in`
in one program, and the golden is what a reader would have written by hand: a
cursor loop over a class, an index loop over a fixed array with the length
written in, and a plain `for` over each spelling of a range. Four things it
holds that nothing else can:

- the **braces**. Each loop is a block of its own, so the three walks over the
  same `bag` declare three cursors and three `x`es and C++ never sees two at
  once. Without them the file does not compile, which is the failure this
  suite exists to catch.
- `int32_t &x = ...` — the loop variable is a **reference**, and the loop that
  writes `x = x * 2` is why the exit status is what it is.
- a `continue` inside a lowered range. The step is in the **head** of a C
  shaped `for`; written at the end of the body it would be jumped over and the
  program would not stop.
- a foreach written **inside a generic**, taken apart in the clone with `T`
  bound and never in the declaration nobody instantiated.

**`a_written_type_takes_a_literal`** — both constructor shapes side by side:
one parameter taking the `Array` (built into a **name** first, because C++ will
not bind a temporary to a reference) and two taking a pointer and a count
(nothing in between). Neither is a C++ implicit conversion.
