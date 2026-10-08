# 0074 — `super` reaches the base's method

Status: **decided and built**, 2026-10-07. Hadley accepted the recommendation
of the second probe round. Closes what record
[0053](0053-super-gives-a-base-its-arguments.md) left open (*"`super.method()`
-- not written, not needed yet, and the syntax is free"*).

| | |
|---|---|
| `super.method()` calls the base's own method **without dispatch**, C++'s `Base::method()` | **decided**, Hadley |
| Only that call skips dispatch: what the base's method calls inside is virtual as ever | follows |
| `super.field` is the object's own field | **decided while building it** |
| `super.method` as a value is refused: through a value the call would dispatch | **decided while building it** |
| `super` outside a class, or in one with no base, is reported | **decided while building it** |

```haard
class C(B):
    def name : i32
        return 100 + super.name()     # B.name, never C.name again
```

## Before

The typer had no case for `super` as an expression: it typed to nothing and
said nothing, and the emitter stopped at *this expression cannot be emitted
yet*.

## How

`super` is the object seen as its base: a pointer to the class's `super`
type, so a `.` after it looks up the base's members and those above. The
emitter writes a method of it as `this->Base::name(...)`, naming the class
that declares the method -- a grandparent's when that is where it is -- and
`self` inside a closure. A field is `this->field`.

Cases: `tests/emitter/cases/super_reaches_the_base_method` (15: the base's
method, a grandparent's whose own call dispatches, a field, inside a closure)
and `tests/type_table/cases/super_needs_a_base`. Three sabotages, each caught.
