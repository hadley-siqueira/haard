# What is pending

Everything not done, of any kind, in one list: bugs, things decided and not
built, decisions waiting on Hadley, things deferred on purpose, the IR's
remaining work and open questions. The records and the agenda
(`docs/design/README.md`) keep the reasoning; this file keeps the list.

Take an item off when it is fixed, decided or dropped, and say where it went.
Last gathered 2026-10-28, after IR stage 4 step 4 (`aed2338`).

## 1. Bugs and gaps: the front end and the C++ path

1. **A closure inside a tuple literal does not help solve a generic's
   parameter.** `apply((|x| -> i32 { ... }, 4))` with `apply<T>` taking a
   `((T) -> T, T)` has to write `apply<i32>`. Record 0067.
2. **A grouped `case` cannot use a bare variant name in a tuple pattern**: it
   is refused as a capture. Record 0072's limitation.
3. **`let low : i8 = -128` is refused** (*128 does not fit in i8*): the
   literal is checked against the type before the unary minus applies, so the
   smallest value of a signed type cannot be written. Workaround in tests:
   `-127 - 1` (`tests/emitter/cases/an_inclusive_range_reaches_the_top_of_its_type`).
4. **Three C++-path gaps found by the IR's step 2** (2026-10-28):
   - a braced literal of braced literals
     (`let g : i32[3][2] = {{1, 2, 3}, {4, 5, 6}}`) passes the front end and
     is emitted as C++ g++ refuses;
   - so is a fixed array copied by `let b = a`;
   - a global whose value reads a global declared **below** it gets no type
     in the front end, while the emitter's comment says it reads zero.
5. **A class value given to a `T&` of its base is copied on the C++ path**
   (found 2026-10-28, IR step 4). `which(d)` with `@b : Base&` is emitted as
   `which(Base(d))` (`Emitter::emit_conversion`, the *not the same class*
   branch): a temporary Base, so a virtual call through the reference runs
   Base's method. Record 0035 says a reference is the thing it names, and the
   IR binds it. Not fixed.
6. **A generic's body is typed with its parameters unbound**, so
   `left = 0` in a `Pair<T>` is reported although every instantiation would
   be fine, and two generics' `K`s are different types there. Records 0036
   and 0067.
7. **The `HashItems.each` workaround** in `std/hash.hd` (walking its cursor
   instead of building the pair) could be simplified now. Low priority.
8. **The name `__tsN` of a template string's temporary is writable** by a
   program, which would collide with it. Record 0032.
9. **An assignment has no type as a value**: `let g = (f = 2)` is *'g' has no
   type the emitter can write*. Record 0057.
10. **A base's own copy**: the emitted copy constructor default-constructs the
    base and then runs the copy `init`, so a derived class's copy has to copy
    what its base holds. Record 0031; check whether `super(...)` (record 0053)
    settled it.

## 2. Decided, not built

1. **The C++ back end and undefined behaviour** (answers 1a, 2b, 3a, 7a, 10a
   in `docs/design/notes/ir-questions.md`). The emitter writes `a + b`, `a / b`
   and `x << n` straight into C++, where signed overflow and an over-wide
   shift are undefined. Haard says overflow wraps, an over-wide or negative
   shift gives the arithmetic result, a zero divisor aborts with a message,
   `MIN / -1` is `MIN`. Also needed: `-ffp-contract=off` (7a) and
   `-ftrivial-auto-var-init=pattern` (10a). And a float converted to a whole
   number it does not fit, or a NaN, **aborts** (Hadley, 2026-10-20): the IR
   interpreter does it, the C++ path does not (a C++ cast is undefined
   there). The tests pass only because g++ runs without `-O`.
2. **Bounds checks** (answer 9b): every index checked, aborting with a
   message. The IR does it for fixed arrays (`check_index`, step 2). Not
   built: the C++ path for fixed arrays, and `Array`/`List`/`String`, whose
   `operator[]` in `std/` should check through an intrinsic that becomes
   `check_index` (`std/array.hd` is `return data[i]` today; every test case
   carries its own copy of the library). A raw pointer stays unchecked.
3. **`volatile`** (answer 11b): library intrinsics
   `volatile_read<T>(p : T*) -> T`, `volatile_write<T>(p : T*, v : T)` and
   `fence()` in `std`, `T` a whole number of 8 to 64 bits, no keyword; the
   compiler recognises them (C++: a `volatile` access; IR: `volatile_load`,
   `volatile_store`, `fence`, which exist). Not built.

## 3. Waiting on a decision

1. **`char - char`**, as in `(c - 'a') as i32`. C++ promotes to `int`;
   Haard's rule is not decided; the IR refuses it (*arithmetic on this
   type*). Two `String` programs of `tests/emitter` wait on it.
2. **Lower only what is reachable** (question 89 already says so). Many
   emitter programs wait in `tests/lowering/not_yet` only because their copy
   of the library has a method taking a closure that nobody calls.
3. **The named return value** (IR step 3, 2026-10-28): when every `return`
   gives back the same local, it is built in the destination and never
   copied, which is g++'s rule. It was never in a record and is observable
   through a copy `init` that does something; both paths follow g++ today.
   To confirm, or to replace with a rule of Haard's own.
4. **Whether record 0018's list composes**: `char*` → `String&` needs two
   steps and gets none. Records 0035, 0037, 0038.
5. **The unary minus against `**`**: `-2 ** 2` is `(-2) ** 2` in Haard and
   `-(2 ** 2)` in Python. Record 0057.
6. **What replaces a `goto` that left several loops** (a labelled `break`,
   say). Record 0078.
7. **The foreign interface** that replaces the natives of `std.low_io`.
   Record 0030 is temporary on purpose and says what replaces it.
8. **How introspection is written in Haard**: the descriptor exists in the
   IR, the spelling is a record of its own. Record 0077, stage 1.
9. **Versions, a registry and a lock file** for packages. Records 0010 and
   0044.
10. **Record 0012 is marked partly decided**: what it left open should be
    checked against what was built since.

## 4. Deferred on purpose

1. **`const`**: a convention for now. Record 0029 (agenda 1.26) writes down
   the semantics and the design so it is not thought out twice.
2. **What `{name: 'Hadley', age: 23}` is** (agenda 1.23): a composite literal
   read by its context; needs compile-time reflection first.
   `docs/design/notes/what-a-brace-literal-is.md`.
3. **Closures that go up** (stored or given back, capturing by value on the
   heap) and **a closure's own unique type**. Record 0058; both additive.
4. **`yield`**, and an **overloaded `def` as a value** (`let f = print`:
   which one?).
5. **A tuple's `to_string()`** written by hand, and **`hash_of` of a tuple**
   (a tuple as a `Hash` key). Record 0067.
6. **Reading a union's variants' payloads** in a pattern. Record 0064;
   additive.
7. **The `.hdm` cache and incremental compilation** (agenda 3.1-3.6, 4.5):
   measured unnecessary for now, 500k lines compile from source in 37s.
8. **Threading** (agenda 4.3).

## 5. The IR (record 0077)

1. **Stage 4, step 5**: enums and `switch`, tuples, unions, closures,
   symbols. Most of the 34 programs in `tests/lowering/not_yet` wait here.
2. **Stage 4, step 6**: the standard library -- `main` taking its arguments
   as a `List`, floats' `**` and `//` (the library's `pow` and `floor`), a
   global of a class type, and the `String` programs.
3. **Questions 25-77 are open** (`docs/design/notes/ir-questions.md`), all
   about the processor's back end and its targets: the ISA and its ABI
   (25-31), memories, scratchpads, placement, the heap and start-up (32-45),
   SIMD and the CGRA (46-54), real time and WCET (55-70), x86-64, ARM, C,
   DWARF and PIE (71-77).
4. **Cycle counting in the interpreter** waits on the timing table
   (question 62).

## 6. The bootstrap (`bootstrap/`, hdc in Haard)

1. **A file system**: listing a directory, canonicalising a path, file size
   and the rest `std::filesystem` gives the C++ compiler (62 uses). The one
   thing left of the five measured on 2026-09-08.
   `docs/design/notes/bootstrapping-haard-in-haard.md`.

## 7. The agenda (`docs/design/README.md`)

Still `open` there, none urgent: 2.3 (the module pool), 3.1-3.3, 3.5, 3.6
(the cache's format), 4.1 (phase order across a group of modules), 4.2 (an
importer whose dependency fails), 4.4 (determinism of merged diagnostics).

Stale, to be brought up to date: **1.15** (`T&&` is built, record 0066),
**2.10** (the expression kinds it lists type now) and **5.3** (the emitter
writes tuples, lists, closures and ranges now) are still marked `open`.
`docs/STATE.md`'s *What does not work* is mostly history, each paragraph
saying when its hole was closed.
