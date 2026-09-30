# 0065 — A struct has no vtable

Status: **decided and built**, 2026-09-29. Hadley: *"em haard, uma struct não
deve ter vtable pois deve sempre ser capaz de modelar dados como plain old
data. Dessa forma, em haard, structs nunca devem ter métodos virtuais"*. The
three questions that followed were answered the same day. Amends
[0020](0020-the-base-chain-in-a-lookup.md) and
[0026](0026-init-and-destroy.md).

| | |
|---|---|
| A struct's methods are **not virtual**, and it has a destructor only when it writes `destroy`, never a virtual one | **decided**, Hadley |
| A struct derives only from a **struct**; a class may derive from a struct | **decided**, Hadley |
| A method of a struct **cannot be overridden**: the same parameters written again below it, in a struct or a class, are an error. `init` and `destroy` are not asked | **decided**, Hadley |
| A struct may write `init`, `destroy` and a copy `init`; it is then not plain, and cannot be the field of a union | **decided**, Hadley |

## What was there

A struct was a class with another word: every method virtual (record 0020), a
virtual destructor always (record 0026), so a vtable pointer in front of the
first field. `struct Point` with two `i32`s was 16 bytes, not 8, and could not
be laid over anything -- which is what a union needs, and what record 0064
found: no struct at all could be the field of one.

## Why each rule

- **No virtual method.** It is the decision itself: a vtable pointer is a
  field nobody wrote.
- **No class above it.** A class has a vtable, and a struct derived from one
  would carry it.
- **A class may derive from a struct.** The class brings its own vtable, and
  the struct inside it keeps its layout. Deleting such a class through a
  pointer to the struct runs the struct's destructor only, as in C++: nothing
  is virtual in a struct to say otherwise.
- **No override.** A method written again below a struct's would be a second
  method, and which one a call runs would depend on the type the caller holds.
  Record 0020 says the same parameters make an override, and a struct cannot
  give one, so it is an error and not hiding. `init` and `destroy` are
  exempt: a constructor and a destructor are not dispatched.
- **`init` and `destroy` stay.** A struct *can* model plain data -- it is not
  required to. One that writes neither, gives no field a value and holds
  nothing that has a lifetime is plain, and record 0064 lets it into a union.

## What it looks like

```haard
struct Point:
    x : i32
    y : i32

    def sum : i32
        return x + y
```

```cpp
struct h2_1_Point {
    int32_t h2_2_x;
    int32_t h2_3_y;
    int32_t m_sum();
};
```

## Where it lives

- The emitter: `emit_method_declaration` writes `virtual` only in a class, and
  `emit_structors` a destructor for a struct or union only when `destroy` is
  written.
- `OverrideChecker::check_layout` refuses a struct derived from a class;
  `check_class` refuses a struct's method written again below it, from the
  base `overridden_by` now gives back; `is_plain` is what record 0064 asks.

## Cases

- `tests/emitter/cases/a_struct_has_no_vtable`: a Point over an i64 in a
  union reads back 3 and 4 as the halves of 17179869187 -- only true with no
  vtable -- plus a struct derived from a struct, a struct running `init`, and a
  class derived from a struct dispatching through its own vtable. Exit 0.
  Sabotaged by writing `virtual` on a struct's methods and on its destructor.
- `tests/override_checker/cases/a_struct_has_no_vtable`: a struct derived from
  a class, a struct's method overridden by a struct and by a class, and what
  stays quiet -- an overload, `init` and `destroy` below a struct, an override
  inside a class derived from a struct. Sabotaged by dropping each refusal and
  the `init` exemption.
- Two emitter goldens lost their structs' `virtual` and empty destructors;
  their exit statuses did not move.
