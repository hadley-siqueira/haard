# 0046 — A conversion into a class is a constructor the compiler calls

Status: **decided and built**, 2026-09-09. Hadley's, and it amends
[0045](0045-a-construction-is-a-call-that-names-a-type.md) the same day 0045
was written.

> I want `takes(p)` to work. A temporary String should be declared out of `p`,
> by handing `p` to String's constructor, and then passed to `takes`.

| | |
|---|---|
| A value reaching a class it can be **constructed** from converts | **decided**, Hadley |
| The rule is an entry on record 0018's list, so all **four places** ask it | **decided while writing it** |
| It knows **no class by name**: it asks whether the class declares the `init` | **decided while writing it** |
| A **value** parameter costs one step, a **reference** two | **kept** from agenda 1.21 |
| Every one-argument `init` is therefore an implicit conversion | **the cost, and it is C++'s** |

## What happened

Record 0045 deleted agenda 1.21's two entries because they knew `String` by its
name, and that was the right thing to delete. But deleting them also took a
program with it:

```cpp
void takes(std::string a);
void takes(std::string& a);

char* p = "abc";
takes(p);            // C++: fine, picks the by-value one
```

Haard said *no 'takes' takes these arguments*. Hadley's answer is that the
conversion should happen — by **building a temporary with the constructor**,
which is what C++ does and what record 0037 already did for a written literal.

The mistake in 0045 was not the deletion. It was reading "the list must not
know a class by name" as "the list must have nothing here at all".

## The decision

A value reaching a class converts when the class declares an `init` taking one
of these. One entry, at the end of `Coercion::steps`:

```cpp
if (builds_from(module, wanted, given)) {
    return to->kind == TYPE_REFERENCE ? 2 : 1;
}
```

`Coercion::builds_from` was already written and already name-free — record
0037 asks it about a written literal — and it is the whole of the mechanism:
it walks the class's own `init`s, takes the ones with exactly one parameter,
and asks whether that parameter is this type. `Coercion::is_string`, which
compared a declaration's name to the text `"String"`, stays deleted.

**Because it is on the list, all four places ask it.** A call, a return, a
binding and an assignment reach `steps`, which is the whole point of the list
living in one place — the lesson of 2026-09-03, when the list reached only a
call. `OverloadResolver::match` lost its special case for a written literal in
the same edit and is one line again: a literal and a value are the same
question now, and the node kind stops mattering.

**The emitter needed nothing.** The tail of `emit_conversion` already writes
`Class(expr)` from the two types, with the `const_cast` a temporary needs to
bind to a plain reference. `by_value(plain)` and `by_value(String(plain))`
come out byte for byte identical, which is the clearest statement of what this
record says: the written construction and the conversion are one mechanism.

### The two costs are what make a call unambiguous

By value costs one and by reference costs two, which is agenda 1.21's pair
kept exactly. It is not decoration:

| in view | C++ | Haard |
|---|---|---|
| `takes(String)` and `takes(String&)`, given a `char*` | picks by value | picks by value |

C++ gets there because a temporary is not viable for a plain `std::string&` at
all, so only one candidate survives. Haard gets there because taking one by
reference costs a step more. Different reasons, same answer — and making both
cost one was tried and reported *this call matches more than one 'takes'
equally well*.

## The cost, written down

This is **C++'s converting constructor**, and C++ defends itself against it
with `explicit`, which Haard does not have. Every one-argument `init` in a
program is now an implicit conversion into its class, and there is no way to
opt one out.

`tests/resolution_table/cases/a_literal_says_which_constructor_it_meant` used
to carry that argument as the reason its last line was refused. The refusal is
gone and the argument is not: it is written here instead, so that `explicit`
— or whatever answers it — is a decision someone makes on purpose rather than
one that gets rediscovered.

## Where Haard still differs from C++

Measured against `/tmp/a.cc`, and after this record the file behaves
identically — `takes(p)` picks the by-value overload and prints `abc`,
`takes(b)` is ambiguous on both sides. What is left is one difference and its
consequences:

**A temporary binds to a plain `T&`.** C++ refuses it; Haard writes
`const_cast<T&>(static_cast<const T&>(T(...)))` and allows it, so a callee can
*write through* a reference to a temporary that dies on the next line.

| | C++ | Haard |
|---|---|---|
| `by_ref(String("abc"))` | refuses | accepts |
| `by_ref("abc")` | refuses | accepts |
| the callee writing through it | refuses | accepts, and it runs |
| a temporary with `T` and `T&` both in view | picks by value | **ambiguous** |

All four are the same fact. It is not a decision about temporaries: it is
[0029](0029-const-is-a-convention-for-now.md) being deferred. C++ makes this
legal for `const std::string&` and illegal for `std::string&`, and Haard has
no `const` to tell them apart — `@a : const String&` does not parse. Without
allowing it, record 0037's own example (`foo("abc")` against
`foo(@s : String&)`) could not have worked at all.

**When `const` stops being a convention, this is the first line that should
change**, and the `const_cast` in `Emitter::emit_conversion` is exactly where
it lives.

## What was rejected

**Putting the entry back under a name**, which is what 0045 deleted. The
program Hadley wants works without it, and a compiler that knows one library
class by name cannot be a compiler two libraries can share.

**Ranking a value and a reference the same.** Simpler, and it makes the
motivating call ambiguous.

**Keeping the literal/value asymmetry** — a written literal converting and a
value not. It was defensible while there was no way to write the conversion,
and record 0045 removed that reason: `String(p)` is writable now, so the
asymmetry was left protecting nothing while still costing a program.
