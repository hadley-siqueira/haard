# 0064 — A union is C's union

Status: **built**, 2026-09-29. Hadley: *"começa pelo union"*, the first item of
the front-end gaps measured on 2026-09-24. No new decision about what a union
**is**: the grammar, the README and every other record already treat `union`
as C's. What this record writes down is what follows from that.

| | |
|---|---|
| A `union` is emitted as a C++ `union`, its fields sharing one piece of memory | **built** |
| A union's methods are **not virtual**, and its destructor exists only when it writes `destroy` | **built** |
| A union derives from nothing, and nothing derives from a union | **refused** |
| A field of a union holds only what has **no lifetime**: a builtin, a pointer, a fixed array of those, an enum that carries nothing, and a struct or union that is **plain** -- no `init`, no `destroy`, no field given a value, and nothing in it or under it with a lifetime | **refused** otherwise |
| At most **one** field of a union is given a value, and that is the one alive when the union is made | **refused** otherwise |

## What was there

The parser, the symbol table and the type phase all knew `union`. The emitter
did not: its three walks over declarations asked for a class, a struct or an
enum. So a union nobody used vanished, and a union reached as a **field** was
emitted by the walk that orders fields -- as a `struct`, its fields side by
side, with a `virtual ~Bits()` declared and never defined. The link failed. Had
the destructor been defined, it would have compiled and run with every field
in memory of its own, which is a union in name only.

## The rules, and why each is a refusal

Record 0047: the compiler refuses when it **cannot decide**, never when it
disapproves. Every rule here is a question nothing in a union can answer,
because nothing in it says which field is alive:

- **A field with a lifetime.** A class, a struct or an enum that carries
  something has to be made, copied and ended by code that knows it is there.
  Haard has no placement `new` to make one by hand and no way to say which one
  to end, and C++ answers the same way: it deletes the union's constructor,
  destructor and copy. A class always has one: its vtable is set by its
  constructor. A struct has none when it is plain ([0065](0065-a-struct-has-no-vtable.md)),
  and a union inside a union the same. A reference is refused too; it has to be
  bound when it is made.
- **Two fields given a value.** Each says *I am alive when this is made*, and
  only one can be.
- **A base, or a derived class.** A method of a union cannot be virtual (C++
  refuses one), so a derived class could override nothing; and a base's fields
  would share memory with the union's own. C++ forbids both.

A tagged enum carrying only builtins has no lifetime either, and is refused for
now: its variants' payloads are not read yet. Allowing it is additive.

## What it looks like

```haard
union Word:
    whole : u32
    bytes : u8[4]

    def low : u8
        return bytes[0]
```

```cpp
union h0_4_Word {
    uint32_t h0_5_whole;
    uint8_t h0_6_bytes[4];
    uint8_t m_low();
};
```

## Where it lives

- `OverrideChecker::check_layout`, `has_no_lifetime` and `is_plain`. In that
  phase and not the type phase because a field of a union may be a struct from
  another module, and what that struct holds is only known once every module
  is typed. The errors carry a caret, and `hdc file.hd` says the same as
  `--emit-cpp`.
- The emitter: `union` in the forward declarations, the type walk and the
  bodies; `emit_method_declaration` takes whether the method is dispatched.

## Cases

- `tests/emitter/cases/a_union_is_cs_union`: memory shared (`258` read back as
  its bytes), a union passed and given back, a method, a field given a value,
  an `init`, a union holding a pointer to itself, another union and an enum.
  Exit 0. Sabotaged by writing `struct` and by writing `virtual`.
- `tests/override_checker/cases/a_union_holds_what_has_no_lifetime`: every
  refusal, a union holding everything allowed -- plain structs included -- and
  one holding four things that are not plain. Sabotaged by dropping the field
  rule, the one-value rule, and each of the three questions `is_plain` asks.
