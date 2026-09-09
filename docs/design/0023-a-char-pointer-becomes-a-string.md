# 0023 — A `char*` becomes a `String`, and a literal ranks by its default

Status: **decided**, 2026-09-03. **Amended 2026-09-08**: the named entry is
now about a `char*` **value** only — a written string literal reaches a class
by record [0037](0037-the-bracket-literal-and-list.md)'s mechanism, which is
not about a name. See the end. Agenda 1.21, which record 0022 opened and left
to [notes/a-char-pointer-where-a-string-was-asked.md](notes/a-char-pointer-where-a-string-was-asked.md).

| | |
|---|---|
| **`String` owns its bytes** — data, size, capacity, and an `init(char*)` | **decided** |
| **`char*` → `String` joins record 0018's list**, and is the first library relation on it | **decided** |
| A literal matching its **default type** costs nothing; any other type it fits costs one | **decided** |
| The list is **one rule of the language**, not four copies, and it applies to a return, a binding and an assignment | **decided** |

## Context

```
def puts : void
    @s : String

puts("abc")
```

Record 0018 has no implicit conversion and its coercion list is closed, so a
`char*` did not become a `String` and this matched nothing. The note framed it
as three questions. What the note missed is the one that decides the other two.

**Rule 1 of record 0018 is not free here, and whether it is depends on what a
`String` is.** `f(3)` against a `u8` parameter emits nothing: the literal is
*born* a `u8`. `puts("abc")` against a `String` parameter is a different animal
if `String` owns a buffer, because then the literal cannot be born one — a
constructor has to run. A non-owning `String` — a pointer and a length, Rust's
`&str`, Go's string — could be built at compile time out of the `.rodata` and
rule 1 would apply literally.

**Hadley, 2026-09-03: `String` owns its bytes.** Data, size, capacity, an
`init(char*)` that allocates and copies, a `destroy` that frees. Mutable, it
grows, and it is the type a program writing text wants.

That settles the shape of the answer: a literal reaching a `String` **runs
code**, so it is not rule 1 dressed up. It is a conversion, and it goes on the
list as one.

## Decision

**1. `char*` → `String` is on record 0018's coercion list.** One entry, which
that record said would need amending it, and this is the amendment.

It is the **first library relation on a list of language relations**, and the
difference is real: the three entries already there — a derived reference or
pointer where a base is expected, `null` where a pointer is expected, one level
of dereference under a `.` — are free at run time and move no bit. This one
names a class the compiler otherwise knows nothing about and runs its
constructor. It is on the list anyway because the alternative is that every
string in every program is written `String("...")`, and the record's own
argument applies: **loosening later is additive.** This is that loosening,
taken deliberately and once, for one type the language names.

**It is not record 0018's rule 4 reopened.** Rule 4 refuses *user-defined*
conversion — any `init(T)` making `T` convertible, the C++ converting
constructor. That stays refused. What is added is one entry naming one type,
the way the language names `Array`; a library cannot add a second by writing a
constructor.

**2. A literal ranks by its distance from its default type.** Record 0018 gives
every literal a type it has when nothing asks — an `i32`, an `f64` — and record
0022 gives a string literal one, a `char*`. Being asked to be that default
costs **0**; being asked to be anything else it fits costs **1**.

This is what makes `write("abc")` pick `write(char*)` over `write(String)`:
`char*` is exact and `String` is a step away. *"First try `char*`; if that is not
possible, become a String"* — Hadley, 2026-09-02 — falls out of the two lists
instead of being written into the ranking as an exception about strings.

It is **rule 5's integer again**, over the default instead of over an
inheritance chain, and it is summed with rule 5's into one score. It is a tie
break and never a filter: a literal that does not fit the default still fits
whatever it fits, and two candidates equally far from the default are still
record 0012's ambiguous call.

**It loosens.** `f(3)` between `f(u8)` and `f(i32)` was an ambiguous call and
now picks the `i32`. Every program that compiled still compiles, which is the
direction record 0018 says is safe.

**3. The list lives in one place, and every question asks that place.** This is
the part no record had said and the code had got wrong.

Record 0018's list is a rule of the **language**, and until 2026-09-03 only a
**call** knew it. A return, a binding and an assignment each compared two types
for equality, so:

```
def up : Base&
    @d : Derived&
    return d            # error: expected Base&, found Derived&
```

failed, and so did `let b : Base& = d` and `b = d`, while `take(d)` worked.
Record 0018 allowed all four and three of them had never heard. The list is now
one class, `Coercion`, and the four places ask it.

## Consequences

- **The compiler knows the name `String`.** Record 0017 already has it knowing
  the name `Array` and nothing else about it; this is the same kind of
  knowledge and not a new kind. It is recognised **by name** until the prelude
  exists and by identity afterwards, and a program declaring its own `String`
  gets its own — which is record 0017's lookup order, already promised for
  `Array`.
- **The emitter has to emit the constructor call.** A `char*` argument reaching
  a `String` parameter is `String(argument)` in the generated C++, and nothing
  records today that the coercion happened — the ResolutionTable keeps a
  literal's chosen type because nothing else could work it out, and a coercion
  is derivable from the two types. Whether the emitter wants it written down
  anyway is an emitter question and is left to the emitter.
- **`String` needs `init` and `destroy` to mean something**, which is agenda
  1.18 and is entirely emitter-shaped. Nothing in the front end changes.
- **A `char*` variable reaches a `String` parameter too**, not only a literal.
  That is what choosing the list over rule 1 buys, and it costs a `strlen` the
  literal did not need — an allocation and a walk, invisible at the call site.
  That invisibility is the price of the entry and it was paid knowingly.
- **The list has a direction.** A `String` does not become a `char*`. Reading
  the bytes out is a method, because it is a question about ownership and not
  about types.

## Rejected

**Leaving it failing** and writing `String(s)` by hand, which is what the note
leaned towards. It is more honest about the allocation and it keeps the list
purely a language one. Rejected because it makes `String` second class next to
`char*` in the one place a program touches it most — every call taking text —
and because the record's own argument says this direction can be taken later
and the other cannot.

**Converting constructors, C++ style.** The general form of the entry, and what
record 0018's rule 4 rejected wholesale. One named entry is not that: a library
cannot opt a type in.

**Marking a string literal as untyped at a call**, which is what the note
proposed for question 1 — carrying it in with no type so each candidate could
ask it to be its own parameter, the way an integer literal is carried. With the
coercion on the list it is unnecessary: the literal is a `char*` like any other
`char*` and reaches a `String` the same way a variable does. One rule instead
of two, and the string literal needs no case of its own anywhere.

## In code

`src/haard/type_table/coercion.{h,cpp}` — the list, and the walk up an
inheritance chain that both the list and the override check read.
`OverloadResolver::match` keeps only what is a call's own: a literal being
asked to *be* a parameter, and its distance from its default.

Cases: `tests/statement_checker/cases/the_coercion_list_is_not_only_for_a_call`
(the four places, six forms allowed and nine refused, including a class with
`String`'s fields and another name), and in `tests/resolution_table/`
`a_string_literal_prefers_a_char_pointer` and
`a_literal_takes_its_default_when_two_fit`, whose goldens print the signature
that won because both candidates resolving is not the question.


## Amended 2026-09-08 — the literal left the list

`char*` → `String` was moved onto record 0037's mechanism, and the two halves
of this record went different ways.

**A written string literal** is no longer a coercion at all. It is a
**construction**: the typer picks the class's `init` taking a `char*`, writes
the choice on the literal, and the emitter writes that call. Any class that
declares such an `init` is reached that way, so the compiler no longer knows a
name for this case — `let note : Note = "note"` builds a `Note`.

**A `char*` value** is still exactly what this record decided: the named
entry, costing one, in `Coercion::steps`. `let s : String = p` is that entry
and nothing else, and a class that is not `String` is not reached by a value.

**The ranking is unchanged and is still this record's**, which was the point
of costing the conversion one step: `write("abc")` between `write(char*)` and
`write(String)` takes the first. `OverloadResolver::match` now costs a written
literal into *any* constructible class the same one step, which is this entry
with the name taken out — so `char* first` still falls out of the list rather
than being an exception written into the ranking.

What is left of this record is therefore one line in `Coercion` and the
ranking rule. Deleting that line is the end of the migration and waits on a
way to write `String(p)` by hand.
