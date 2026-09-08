# A `char*` reaching a `String` — four examples, and what is open

Rewritten 2026-09-06 from Hadley's own examples. The earlier draft talked about
"migrating" and named nothing concretely; this one goes example by example and
shows the C++ that comes out.

## The rule, as Hadley stated it

> Only `char*` literals exist. When a `char*` is given to something that
> expects a `String`, the appropriate **constructor** or the appropriate
> **overloaded operator** is called.

So `String` is never written by a literal. It is *built* from one, by a call
the compiler chooses and writes down — which is record
[0037](../0037-the-bracket-literal-and-list.md)'s mechanism, and record
[0034](../0034-an-operator-is-a-method-with-an-unwritable-name.md)'s rule that
the emitted C++ must not lean on C++ having the feature.

## The four examples

### 1. `let a = "abc"`

`a` is a `char*`. **Already true**, record 0022, and nothing changes.

```cpp
char *h0_1_a = "abc";
```

### 2. `let a : String = "abc"`

Wanted: `String("abc")`, hidden from the author.

**Today** it compiles, and the C++ is

```cpp
h3_1_String h0_4_a = "hello";
```

which names no constructor. It works because record 0026 emits `init` as a C++
constructor of one argument and **C++'s own converting constructor** runs it.
Haard decided *that* it converts; C++ decided *how*.

**Wanted**

```cpp
h3_1_String h0_4_a("abc");
```

### 3. `foo("abc")`, where `foo` takes a `String&`

**Today** it is refused:

```
error: no 'foo' takes these arguments
```

because `char*` → `String` and value → reference are **two** entries of record
0018's list and the list is a list of single steps (record 0035 left that
open).

**Wanted**: a `String` is built and passed by reference. C++ will not bind a
temporary to a non-`const` reference, and Hadley already chose the way out for
`List` — a **named local**, hoisted before the statement:

```cpp
h3_1_String __st0("abc");
int32_t h0_5_n = h0_1_foo(__st0);
```

Which also answers record 0035's open question **without** making the list
compose: the compiler is not chaining two coercions, it is writing a
construction.

### 4. `let a : String` then `a = "abc"`

**Today** this compiles, and it is the example that shows the cost best:

```cpp
h3_1_String h0_4_a;
h0_4_a = "abc";
```

C++ builds a **temporary `String`** from `"abc"` and then runs record 0031's
copy assignment on it — two allocations and a copy, for what should be one
call.

**Wanted**: `String` declares `def operator= : void` taking a `char*`, and

```cpp
h0_4_a.m_operator_eq_pb12("abc");
```

**`=` is not overloadable today.** `def operator=` is *expected an operator
that may be overloaded, found '='* — record 0034 left it out on purpose,
because record 0031 already gives a class a copy assignment and something has
to say which wins.

## Decided and built, 2026-09-06

| | chosen |
|---|---|
| **Open 1, how wide** | **1a** — the pair `char*` → `String`, known by name. `Coercion::is_string` stays |
| **Open 2, `operator=`** | **2b** — `a = b` is `a.m_assign(b)` and the C++ `operator=` is gone |
| **Open 3, the temporary** | a C++ temporary with the `const_cast` the emitter already writes |

Hadley's reason for 1a: *the programmer can always write an overload that calls
another one and convert by hand.* A `Path` that wants what `String` has writes
`abrir(Path)`, `abrir(char*)` and `abrir(String&)` and has each call the right
one — a few lines in the library, instead of a rule that fires silently for
every class in the program.

The temporary is a C++ one and **not** a hoisted local, unlike the array
literal: nothing here is recognisable before the type phase, so the sugar pass
cannot hoist it. The `const_cast` is the one record 0031's copy constructor
already needs, for the same reason — Haard has no `const` to write on a
parameter (record 0029).

### The four examples, as emitted

```cpp
char *h0_16_plain = "abc";                        // 1
h0_1_String h0_17_built("abcd");                  // 2
by_reference(const_cast<String&>(static_cast<const String&>(String("abcde"))));
by_value(h0_1_String("abcdef"));                  // 3
h0_17_built.m_operator_eq_pb12("abcdefg");        // 4
```

`char*` → `String&` is **one more entry** on record 0018's list and not the
list learning to compose, which record 0035 still leaves open. It costs two, so
a `foo(char*)` beside a `foo(String&)` still gets the literal.

### Two things it found

**A type index was read in the wrong module's table**, for the fourth time in
this project. A parameter's type belongs to the table of the module that
**declared** the function and it was being compared against one from the
caller's — which turned `__io_open_read(path)` into
`__io_open_read(File(path))`. The same class is the same **pair**, which record
0016 writes down, and never the same index.

**A value a call gave back could not be passed by reference.**
`append(gives())` where `gives` returns a `String` is an rvalue against a plain
`&`, which C++ refuses. Broken before any of this and simply never written,
since nothing had returned a class by value into a `&` parameter.

## What is open

Three things, and the third is small.

### Open 1 — how wide is "something that expects a `String`"

**1a. The pair, by name.** `char*` → `String` stays a **named** library
relation, exactly as record 0023 made it, and only the **emission** changes:
the compiler finds `String`'s `init(@text : char*)` and writes the call.
`Coercion::is_string` stays.

*For*: nothing else in the language moves. Examples 2, 3 and 4 all work.
*Against*: the compiler still hardcodes the name `String`, and the next library
relation is a second hardcoded name.

**1b. Any class with a matching one-parameter constructor.** `is_string` is
deleted, and `char*` → `String` falls out of a general rule — the same rule
that already lets `List<i32>` take an `Array<i32>`.

*For*: one rule, no names in the compiler, and record 0018's list stops growing
library relations.
*Against*: it fires **silently for every class in the program**. C++ has
`explicit` to defend against exactly this; Haard would have nothing. Record
0018's posture is a **closed** list, so that what converts into what is
readable in one place.

**1c. 1b, but the class opts in.** A class marks which constructor may run
implicitly. `String` marks its `char*` one, `List<T>` marks its two, and
nothing else converts by accident.

*For*: 1b's single rule without 1b's surprise, and it is the safer default —
C++ chose implicit-by-default and then had to add `explicit`, which is the same
decision made twice in the wrong order.
*Against*: it needs a spelling. This project's habit is a **name** the language
looks for (record 0026's `init`/`destroy`/`copy`, record 0031's `to_string`),
so the natural shape is a second method name — `def from` beside `def init` —
rather than a keyword on the `init`.

### Open 2 — a user's `operator=` beside record 0031's copy assignment

Both are `operator=` on the same class, and they are told apart by their
parameter, which is what overloading already does:

| written | means |
|---|---|
| `a = "abc"` | the user's `operator=(char*)` |
| `a = other_string` | record 0031's copy assignment |

**2a. Look for a user's `operator=` first; fall back to record 0031.** The
right side's type picks, exactly as record 0034 already picks among `append`s.
Record 0031's copy assignment is untouched.

**2b. 2a, and record 0031's copy assignment stops being a C++ `operator=`
too.** Today the emitter writes a real C++ `operator=` for it, which is C++
operator overloading — the thing Hadley banned for user operators, surviving in
the compiler's own emission. Under 2b, `a = other_string` becomes
`a.m_copy(other)` and the C++ `operator=` disappears.

*For 2b*: one rule with no exception, and `a = b` between two classes stops
depending on C++'s rules.
*Against 2b*: record 0031's copy assignment also **destroys first**, and doing
that at every use site instead of once inside a C++ `operator=` is more emitted
code for no behaviour change.

### Open 3 — where the temporary of example 3 goes

Carried over from `List`, unless Hadley says otherwise: a **named local**,
hoisted before the statement, which brings record 0032's three refusals with it
— a `char*` reaching a `String&` inside a loop condition or on the right of
`and` would be refused.

The alternative is the `const_cast` the emitter already writes for record
0031's copy constructor. It hoists nothing, so it needs no refusals — at the
cost of a cast in the output that a reader has to trust.

## What each option costs to build

| | 1a pair by name | 1b general | 1c opt in |
|---|---|---|---|
| example 2 | the emitter writes the call | same | same |
| example 3 | a hoisted `String` local | same | same |
| example 4 | `=` on record 0034's table | same | same |
| `is_string` | stays | **deleted** | **deleted** |
| new syntax | none | none | a name or a keyword |
| risk | a second hardcoded name later | silent, every class | one more thing to write |

`=` on record 0034's table is needed by **all three**, and so is the hoisted
temporary. The only thing the three disagree about is **which pairs** convert.
