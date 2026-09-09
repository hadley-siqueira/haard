# 0050 — `isize` and `usize`, the width of a pointer

Status: **decided and built**, 2026-09-09. Hadley asked for the pair; the
spelling of the signed one was decided after measuring what the other spelling
would cost.

| | |
|---|---|
| Two builtins whose width is a pointer's: `isize` signed, `usize` not | **decided**, Hadley |
| They emit `intptr_t` and `uintptr_t`, not `ptrdiff_t` and `size_t` | **decided while writing it** |
| They are on record 0049's list as what a pointer converts to | **decided while writing it** |
| The signed one is **not** called `size` | **decided**, Hadley, after measuring |

## Why not `size`

`size` was what was asked for, and it cannot be a keyword. It is the field
name of `String`, `List`, `Hash` and `Array`:

```haard
class String:
    data : char*
    size : i32
    capacity : i32
```

**727 uses in this repository** — 68 in `std/`, 563 in `tests/programs`, the
rest elsewhere — and every one of them would have had to be renamed. Worse,
every program anyone has ever written with a `size` field would stop parsing,
and no deprecation is possible for a word that becomes a keyword.

`isize` and `usize` are Rust's pair for exactly this and collide with nothing:
zero uses of `isize` anywhere. `iptr`/`uptr` was the other candidate and says
what they are more precisely; it was declined because nobody arriving from
another language would recognise it.

## Why `intptr_t` and not `size_t`

The names say *size*, and `<cstddef>`'s `size_t` and `ptrdiff_t` are what a
size and a difference are in C++. `<cstdint>`'s `intptr_t` and `uintptr_t` are
what a **pointer fits in**.

They are the same width everywhere that matters, so the choice is about which
promise Haard is making. It is the second one: what makes these two worth
having is record 0049's list, where a pointer converts to an integer it fits
in. `intptr_t` says that in the type's own name, and it needs no header the
emitter was not already including.

So `holds_a_pointer` now answers *yes* for `isize` and `usize` first, and for
`i64` and `u64` after — those two were on that list before this pair existed
and sixty-four is what Haard assumes, having no target model at all.

## What it touched

Fourteen places, and the shape of the work is worth recording because the next
builtin will touch the same ones: the enum, the two name tables in the
compiler, `builtin_of`, `is_builtin_type`, the token kinds and their names, the
scanner's keyword list, the emitter's C++ names, `limit_of` (what a literal
asked to be one of them may hold), `is_a_number`, `holds_a_pointer`, and the
library's `append`, `print` and `println`.

**`usize` cannot borrow the signed path**, for the reason `u64` cannot: past
`i64`'s largest it would come out negative. It gets `u64`'s own digit walk,
and the case's golden holds `18446744073709551615` to prove it.

## Two things it broke, and both were the same shape

**A position standing in for a meaning.** Three places asked *is this a whole
number* by writing `which <= BUILTIN_I64`. Two integers added after `i64` in
the enum would have been silently classified as not-integers. The three are
now one function, `is_a_whole_number`, next to the enum it reads.

**Two test dumpers keep their own copy of the builtin name table.**
`tests/type_table/types.cpp` and `tests/resolution_table/resolutions.cpp` each
hold an array of the names, indexed by the enum, and neither is built from the
compiler's. Adding two builtins shifted every name past them: the whole
type_table suite failed reporting `f64` as `void` and `bool` as `char`. The
copies are updated, and they are still copies — a third would break the same
way.

Two goldens moved for reasons with no meaning in them, and both were checked
line by line before being accepted: the type_table goldens count **interned
types**, which went up by two in every case, and the emitter's goldens carry
**mangled names**, which encode the builtin's index — `m_poke_b12` became
`m_poke_b14` because `char` moved from 12 to 14. The emitted programs still
compile and still give the same exit status, which is that suite's verdict.

## What is not decided here

Whether `sizeof` should give back a `usize` instead of a `u64`. It gives a
`u64` today and this record did not change it. On every target Haard emits for
they are the same width, so nothing is wrong; on the day one of them is not,
`sizeof` is answering with the wrong one.
