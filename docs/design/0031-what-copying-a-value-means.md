# 0031 — What copying a value means

Status: **decided**, 2026-09-05, and implemented. **Amended 2026-09-06 by
[0038](0038-everything-about-construction-is-init.md): there is no method
named `copy`.** Everything this record decides still holds — `destroy` says a
class owns something, one that owns and says nothing about being copied may not
be, and it is reported at the four places — but *what says it* is now an
**`init` whose one parameter is the class itself**, because everything about
construction is `init` and there will not be a second name for any of it. Read
`copy` below as that overload.

Status: **decided**, 2026-09-05, and implemented. Hadley, asked while planning
template strings and answered before them, because it is what blocks them.

| | |
|---|---|
| A class that declares `destroy` **owns** something | **decided** |
| One that owns something and declares no `copy` **may not be copied**, and that is reported | **decided** |
| `copy` is the **third** of record 0026's family, emitted as a C++ copy constructor **and** copy assignment | **decided** |
| Everything else copies as it always did | **unchanged** |
| A user's class in a `${}` is found by a **`to_string`** convention | **decided**, for when template strings land |

## The bug this record exists because of

```haard
class Owner:
    data : char*

    def init : void
        data = new char[4]

    def destroy : void
        delete[] data

def by_value : char
    @o : Owner          # by value
    return o.data[0]
```

```
free(): double free detected in tcache 2
AddressSanitizer: attempting double-free
    #1 h0_1_Owner::m_destroy()
```

Nothing was reported. Haard had **no decision at all** about what copying
means, so the emitted C++ got the memberwise copy a C++ compiler writes when
nobody says otherwise — the pointer duplicated, and both destructors run.

It reached this record from the other end. A template string is a `String`
(record 0022), a `String` owns its bytes (record 0023), and every shape the
sugar could take — a builder that gives one back, or a chain of `concat` —
passes a `String` **by value**. So the blocker was never template strings.

## Decision

**1. `destroy` is what says a class owns something.** Record 0026 already
makes it the destructor, so a class with one is a class with something to
release. Nothing new is written down to mark ownership.

**2. A class that owns something and has not said how to be copied may not
be.** Reported, by name, at the place the copy is written:

```
error: Owns cannot be copied, and this parameter takes one by value
  --> main.hd:51:22
   |
51 |     let a = by_value(owner)
   |                      ^^^^^
```

Refusing rather than copying is the same choice record 0028 made about a class
that cannot be built: say so about it, rather than emit C++ that a compiler
would refuse in words about a line nobody wrote — or worse, here, C++ that
compiles and corrupts the heap.

**3. `copy` is the third of record 0026's family.** An ordinary method, and
emitted twice:

```haard
def copy : void
    @other : Owner&

    size = other.size
    data = new char[size]
    # ... take the bytes
```

becomes a C++ **copy constructor** and a **copy assignment**, both a call to
it. Two details, and both are forced:

- **`const_cast`.** C++ wants `const Owner&` or the copy constructor will not
  bind to a temporary, and Haard has no `const` to write on the parameter
  (record 0029, deferred). The emitter bridges it and Haard never sees it.
- **Assignment destroys first.** `copy` is written as *become a copy of that*
  by a class holding nothing, so an assignment has to put the target back into
  that state before calling it. With a guard for `a = a`, which would
  otherwise free what it is about to read.

**4. The four places, and they are the four record 0018 already knew.** A
call's argument, a return, a binding and an assignment. The question lives in
one place — `Coercion::may_be_copied` — and each of the four reports it in its
own words with its own caret, which is what record 0018's list learned to do
on 2026-09-03.

The call's is asked in `ExpressionTyper::call` and not in the resolver,
deliberately: the resolver ranks and never reports, so a candidate taking an
uncopyable class by value would simply not match and the reader would be told
that no overload takes these arguments — true, and about the wrong thing.

**5. A base's ownership is the derived class's.** The walk goes up the chain,
so a class deriving from one that owns is uncopyable unless it says otherwise.

**6. Everything else is untouched.** A `struct` of plain fields owns nothing
and is copied by all four ways exactly as before. A pointer and a reference
name a thing rather than holding one.

## And the convention this unblocks

Hadley, 2026-09-05, deciding it early so the shape is settled: a class of the
user's inside a `${}` is turned into text by a **`to_string`** method the
compiler looks for **by name** — the same kind of knowledge record 0026 has of
`init` and `destroy` and record 0030 has of `std.io`'s eight. A class without
one cannot be interpolated, and that is reported.

## It amended record 0018's list

Following this record to its conclusion — *hold it by reference and nothing is
copied* — ran straight into a wall: **record 0018's list had no entry for a
value where a reference was expected.** So `f(String&)` could not be called
with a `String`, and the advice this record gives had nowhere to go.

It is not a conversion. The parameter **is** the value, nothing happens at run
time, and a derived value binds to a base's reference for the reason the list
already allowed it between two references — nothing is sliced, because nothing
is copied. An upcast **by value** is still refused, which is the slicing the
list was always about.

Only in that direction. A reference where a **value** was expected is a copy,
and it is this record's subject.

## And `String` is its own builder

Written while proving the above, and it settles a question the template string
planning had left open. **There is no `StringBuilder` and there does not need
to be**: what one would hold is exactly what a `String` holds, and what it
would do is `append`. So `String` carries the overloads — `char`, `char*`,
`String&`, `i64`, `i32`, `u32`, `bool`, `f64` — and the Ast → Ast pass will
write calls to those.

One name for the compiler to know instead of two, and it is the name records
0022 and 0023 already had it knowing.

## What is still open, named

- **Moving.** There is none, so `gives_back()` copies where C++ would move.
  Correct and not free. It is additive: a `move` alongside `copy` would only
  ever loosen.
- **A base's own copy.** The emitted copy constructor default-constructs the
  base subobject and then calls `copy`, so a derived class's `copy` has to
  take care of what its base holds. `super(...)` is undecided (record 0026)
  and this is the same gap.
