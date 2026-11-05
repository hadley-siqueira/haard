# What is pending

Everything not done, of any kind, in one list: bugs, things decided and not
built, decisions waiting on Hadley, things deferred on purpose, the IR's
remaining work and open questions. The records and the agenda
(`docs/design/README.md`) keep the reasoning; this file keeps the list.

Take an item off when it is fixed, decided or dropped, and say where it went.
Last gathered 2026-10-28, after IR stage 4 step 4 (`aed2338`); brought up to
date 2026-11-05, after step 2 of record 0084.

Taken off on 2026-11-04: *stage 4 step 6* (built, record 0077), and *a `char`
past 127 reads differently on the two paths* (a `char` has no sign, record 0081),
*a `char` past 127 cannot be written as a literal* (record 0082), and *the C++
back end and undefined behaviour*, *bounds checks* and *what `String` does out of
range* (`s[i]` stops, `at` gives `'\0'`; record 0083).

Taken off on 2026-11-03: *stage 4 step 5* (built), *`char - char`* (record 0079), *lower only what is
reachable* (built, question 89), *the named return value* (record 0080),
*whether record 0018's list composes* (already answered by record 0046: a
value reaches a `T&` of a class it builds, at a cost of two; only the open
lines of records 0035, 0037 and 0038 had not been updated), *the unary minus
against `**`* (record 0057 amended), and *a class value given to a `T&` of
its base is copied on the C++ path* (fixed in `Emitter::emit_conversion`,
case `a_derived_class_is_its_base_by_reference`).

## Where we stopped (2026-11-05)

The assembler and the runtime of record 0084 are built, in `toolchain/`, x86-64 only (steps 1
and 2 of the record's order), and in `make check`. Next in that order: **the back end in `hdc`,
running `tests/emitter` natively** (step 3). The other choice is a round of the small bugs of
section 1.

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
5. **A generic's body is typed with its parameters unbound**, so
   `left = 0` in a `Pair<T>` is reported although every instantiation would
   be fine, and two generics' `K`s are different types there. Records 0036
   and 0067.
6. **The `HashItems.each` workaround** in `std/hash.hd` (walking its cursor
   instead of building the pair) could be simplified now. Low priority.
7. **The name `__tsN` of a template string's temporary is writable** by a
   program, which would collide with it. Record 0032.
8. **An assignment has no type as a value**: `let g = (f = 2)` is *'g' has no
   type the emitter can write*. Record 0057.
9. **A base's own copy**: the emitted copy constructor default-constructs the
    base and then runs the copy `init`, so a derived class's copy has to copy
    what its base holds. Record 0031; check whether `super(...)` (record 0053)
    settled it.
10. **A `char*` value bound to a `T&` of a class on the C++ path**:
    `let s : String& = p`, `p` a `char*`, passes the front end (record
    0046) and is emitted as `String& s = p`, which g++ refuses (a temporary
    to a plain reference). The IR builds and holds the temporary. The same
    as an argument works. Found 2026-11-03.
11. **A number literal given to a class by its `init`**: `by_value(5)` with
    `by_value(m : Meters)` and `Meters.init(n : i32)` is *no 'by_value'
    takes these arguments*, while `by_value(n)` with `n : i32` works.
    Record 0046 says a literal and a value are one question. Found
    2026-11-03.
12. **A temporary assigned to an enum that carries a class, on the C++
    path**: `copy = Box.Empty` is emitted as `copy.m_assign(Box_Empty())`,
    a temporary to a plain reference, which g++ refuses; through a name it
    works. Found 2026-11-03.
13. **Four things the front end says, found 2026-11-04**, none decided:
    - a closure at module level (`let twice = |x : i32| -> i32 { return x * 2 }`)
      is *cannot find 'x' in this scope*, where one inside a function works;
    - a tuple converts element by element when assigned (`w = make()`, a
      `(char*, i32)` into a `(String, i32)`) and not when bound
      (`let k : (String, i32) = make()` is *expected (String, i32)*);
    - `let r : i32&` with no value passes it, and g++ refuses what comes out;
    - there is no `**=`, where `//=` and `>>>=` exist (record 0057).

## 2. Decided, not built

1. **`volatile`** (answer 11b): library intrinsics
   `volatile_read<T>(p : T*) -> T`, `volatile_write<T>(p : T*, v : T)` and
   `fence()` in `std`, `T` a whole number of 8 to 64 bits, no keyword; the
   compiler recognises them (C++: a `volatile` access; IR: `volatile_load`,
   `volatile_store`, `fence`, which exist). Not built.

2. **The native back ends** (record 0084, questions 71-77 and ten more): `hdc` writes
   text, `hasm` makes a static ELF, x86-64 Linux first, direct system calls, no C. The
   first goal is a correct result, not speed. **Steps 1 and 2 of 6 are built** (`toolchain/`,
   2026-11-05). Left: the back end in `hdc`, the bootstrap's steps, ARM64 and the
   processor, debug information. How a label names its root is still to be spelled, and
   whether the runtime's allocator and files move to Haard over a `__syscall` native.
3. **PIE as the default on Linux, before a public version** (question 77). The first
   version loads at a fixed address, with the code already position independent.

## 3. Waiting on a decision

1. **The rest of a character's arithmetic** (record 0079): `c + d`,
   `c * d` and the rest between two characters are still typed as a `char`
   (and `c += d` takes one), and the IR refuses them.
2. **What replaces a `goto` that left several loops** (a labelled `break`,
   say). Record 0078.
3. **The foreign interface** that replaces the natives of `std.low_io`.
   Record 0030 is temporary on purpose and says what replaces it.
4. **How introspection is written in Haard**: the descriptor exists in the
   IR, the spelling is a record of its own. Record 0077, stage 1.
5. **Versions, a registry and a lock file** for packages. Records 0010 and
   0044.
6. **Record 0012 is marked partly decided**: what it left open should be
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
9. **A debug format and a debugger of our own**: interesting in the future, since
   they could know everything about Haard, enums and generics included, where DWARF
   describes them as C's concepts (Hadley, 2026-11-04). DWARF first. Record 0084.

## 5. The IR (record 0077)

1. **`__pow` and `__floor` become `pow` in Haard and the `ffloor` instruction**
   (Hadley, 2026-11-04, record 0084), replacing what stayed native for now. `pow`
   needs a module most cases' trimmed copies of `std/` lack, and goldens may change
   in the last digit. Not built.
2. **Building a variant's payload makes one copy fewer in the IR**:
   `Box.Full(Held(7))` builds the `Held` in place, the C++ path gives a
   temporary to the variant's maker and copies it in. Seen only through a
   copy `init` that does something; record 0080's question one construct
   over. Found 2026-11-03 (`an_enum_ends_and_copies_what_is_alive`).
3. **Questions 25-70 are open** (`docs/design/notes/ir-questions.md`), all about
   the processor's back end: the ISA and its ABI (25-31), memories, scratchpads,
   placement, the heap and start-up (32-45), SIMD and the CGRA (46-54), real time
   and WCET (55-70). Questions 71-77 were answered on 2026-11-04.
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
