# 0052 — A range is a value, and the value is a `Range<T>`

Status: **decided and built**, 2026-09-09. Hadley's.

| | |
|---|---|
| `0..10` written anywhere but a `for` is a **`Range<T>`** | **decided**, Hadley |
| It holds a **start, a stop and a step**, like Python's | **decided**, Hadley |
| `stop` is **one past the end**, so `..` arrives with the end plus one | **decided while writing it** |
| An `Array<T>` is built from one by a **constructor** | **decided**, Hadley |
| Inside a `for ... in` **nothing of this happens** | **kept** from record 0040 |

## What was missing

`0..10` was the last expression kind that typed to nothing — the one entry
left on agenda 2.10's list after a symbol left it on 2026-09-08. `let r =
0..10` passed `hdc` in silence and died in the emitter with *'r' has no type
the emitter can write*.

Record 0040 had already read the two node kinds as **syntax**: a `for ... in`
over a range is taken apart before anything is typed and becomes a plain C
shaped loop. That is still exactly what happens, and it is why walking a range
costs nothing and allocates nothing. This record is about the same expression
written anywhere else.

## The class

`std/range.hd`, generic over the whole number it counts in, owning nothing —
so no `destroy` and no `copy`: record 0031's question is about a class that
owns memory, and this one is three integers side by side.

```haard
class Range<T>:
    start : T
    stop : T
    step : T
```

with `length`, `at`, `contains`, `first`/`past_the_end`/`stride`, `==`, `!=`,
and record 0040's three names — `iterator`, `has_next`, `next` — so a range
held in a binding is walked exactly as a container is.

`RangeCursor<T>::next` gives back a **`T` and not a `T&`**, which is the one
place a range differs from every other container: there is nothing to write
through, because the values do not exist until they are asked for.

### `stop` is one past the end

Python's shape, and it is what makes `length` a subtraction. Haard's two
spellings both arrive through the same field: `..` **includes** its end
(record 0040's rule, Ruby's convention), so `0..4` is built with a stop of 5,
and `0...4` with 4. The two lengths — 5 and 4 — are the whole of the
difference and are what the case pins.

### The type is found by name

`Range` is looked for in the scope the range was written in, exactly as
`Array` is for `[1, 2, 3]`: `TypeBuilder::build_generic`, the same call, the
same rule. So a program that declares its own `Range` gets its own, a program
that writes no import is told *a range is a Range<T>, and 'Range' names
nothing here*, and the compiler knows the name and nothing else about it.

`Array<T>` gained an `init(@from : Range<T>&)`, so `Array<i32>(2..6)` is the
written form of turning the values a range only describes into values that
exist.

## What writing it found, and it had nothing to do with ranges

**A generic cloned during inference was never given its own type**, when the
generic was declared in the module that instantiated it. `this` inside the
clone's own methods read `INVALID_TYPE` and came out as `<none>*`:

```
error: cannot assign <none>* to Span<i32>*
  --> main.hd:50:21
   |
50 |         cursor.held = this
```

— about a method of a class that is fine, in a file whose author wrote no
`Span<i32>` anywhere near it. From **another** module it never showed, because
the walk had already run; a written type never showed either, because that
clone is made during collection. It needed a generic declared and instantiated
in one module, inferred rather than written. Nobody had done that.

It is record 0039's shape and record 0045's fix, one level up:
`TypeCollector::type_signature_now` types a **class** candidate too now — its
own type, and its methods' signatures — and `TypeBuilder` calls it at both
places an instantiation happens, the moment the instantiator hands a clone
back. Two goldens moved from `<none>` to a real type as a result, which is the
fix showing.

**And `as` had to learn about an unbound parameter.** `(i as T)` inside
`Range<T>` was refused by record 0049's list, which has no entry that could
answer about a type parameter. Record 0002's rule applies: what is written
inside a generic nobody instantiated is not a program yet, so the cast says
nothing and the clone asks again with `T` bound.

**A range is an rvalue.** `Array<i32>(2..6)` handed a temporary to a `Range&`
and g++ refused it. `Emitter::is_an_rvalue` knew about a call and the two
literals; a range written as a value is a construction the emitter writes in
place, so it arrives as itself and belongs on that list.
