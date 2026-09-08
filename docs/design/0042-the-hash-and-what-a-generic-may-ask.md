# 0042 — `Hash<K, V>`, and what a generic body may ask of its parameter

Status: **decided and implemented**, 2026-09-08. Hadley: *implemente a classe
`Hash<K, V>` e coloque ela como parte do prelúdio que nem é Array, String etc.*

| | |
|---|---|
| `Hash<K, V>` is written **in Haard**, like the other three | **decided**, record 0022 |
| Hashing a key is an **overload set**, `hash_of`, and not a method | **decided while writing it** |
| It is **open addressed**: three parallel buffers and linear probing | **decided while writing it** |
| Walking one gives back its **keys** | **decided while writing it**, record 0040 |
| It is in the **prelude**, beside `Array`, `List`, `String` and `File` | **decided**, Hadley |

## How a key is hashed, with no interfaces

There are no interfaces (agenda 1.16, and Hadley closed it: *they do not
exist*), so a generic cannot say *K must know how to hash itself*. There is no
`where`, no trait, no bound.

What there is, is **overloading**. `hash_of` is a free function with one
overload per key type, and record 0002's clone resolves the call against the
`K` it was instantiated with:

```haard
def hash_of : u32
    @key : i32

def hash_of : u32
    @key : char*

def hash_of : u32
    @key : symbol          # record 0041, by the text its entry holds

def hash_of : u32
    @key : String&
```

A key type nobody wrote one for is reported **at the instantiation** — *no
'hash_of' takes these arguments* — which is the same answer overloading gives
everything else, in the same words. Adding a key type is adding an overload,
from anywhere, and needs nothing from the compiler.

`u32` and not `i32`, because hashing **multiplies**: an i32 that overflows is
undefined where an unsigned one wraps, and wrapping is the whole of what mixing
bits is. The bucket takes it back down once, in `slot_of`.

## Open addressing, and why not buckets

Three parallel buffers — `keys : K*`, `values : V*`, `states : u8*` — and
linear probing. One allocation per buffer instead of one per entry, no node
class, no `K**`, and record 0028's `new K[n]` is the whole of what it needs.

A removal leaves a **tombstone** (state 2) so a probe walks past it, and the
table grows at half full by rehashing into buffers twice the size.

It owns three buffers, so record 0031 makes it write a copy `init` — and that
is checked by the case: a copy holds its own.

## Walking one gives back the keys

Record 0040 asks a container for `iterator()`, `has_next()` and `next()`, and
`HashCursor<K, V>` walks the slots, skipping every one that holds nothing.

`next` gives back a **`K&`**, so `for k in h` binds each key and the value is
one subscript away — `h[k]`. Giving back a pair would need a pair type, and a
tuple is not emittable yet.

## What writing it found in the compiler

Two things, both the shape this project keeps finding: something that compiled
in Haard and was refused by g++, or reported about a program that is fine.

### A call inside an unbound generic waits for the clone

`hash_of(key)` inside `Hash<K, V>` matched **no** overload while `K` was still
a type parameter, and the type phase reported it — *no 'hash_of' takes these
arguments*, about a class that is fine. Record 0002 says what is written inside
a generic nobody instantiated is not a program yet, and the statement checker
and the emitter already skip one whole; this is the type phase learning the
same rule for the one thing in it that reports.

So: a call whose argument is a **type parameter** and that matched nothing says
nothing. It is asked again in the clone, where every argument is concrete, and
reported there.

### An operator's operand is an argument

```haard
let printed = named_state == "estado: ready"
```

Haard accepted it and **g++ refused it**: `operator==` takes a `String&` and
the emitter handed it a raw `char*`. It is the copy assignment's bug of the
same morning, one method over — `emit_operator` did not run its operand through
`emit_conversion`, which is what builds the class and gives the temporary a
name to bind to. Record 0034 makes an operator a method; this makes its operand
an argument.

It also found the half of record 0037 that was inconsistent: `binary` types the
right operand against the **left**, so a string literal was already a
construction of the class on the left, and then the ranking typed it a `char*`
again to rank it — leaving the type and the constructor on that literal
disagreeing. Both are written down once more against the parameter that won,
and through a reference, since what a `String&` takes is a `String`.

## In the prelude

`import std.hash` joins the four already there. Every module of a program that
carries the table's prelude block gets `Hash` and the `hash_of` overloads
without writing an import, which is what record 0033 is for.

## In code

`std/hash.hd` — the class, its cursor and the four `hash_of` overloads.
`String.append(symbol)` came with it, so `"${:ready}"` is the symbol's name
(record 0041).

The case is `tests/programs/cases/a_hash_is_written_in_haard`: i32, `String`
and **symbol** keys, growth past four rehashes, a removal probed past, a copy
that holds its own, and `for k in h` adding the values its keys reach.
