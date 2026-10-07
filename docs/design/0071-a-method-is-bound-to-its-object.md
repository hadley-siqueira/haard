# 0071 — A method is bound to its object

Status: **decided and built**, 2026-10-07. Hadley, of the options for `let f
= c.soma`: a **bound method**, sugar for a closure that captures `c` by
reference, with record [0058](0058-a-closure-captures-by-reference.md)'s rules
-- it may dangle, and that is trusted (record
[0047](0047-what-the-language-expects-of-the-programmer.md)). The unbound
form, `Contador.soma` taking the object as its first argument, is not now. An
overloaded method as a value needs the expected type to choose.

| | |
|---|---|
| `c.soma` where a value goes is a function value bound to `c`, which it **refers** to | **decided**, Hadley |
| `Contador.soma`, unbound | **not now**, Hadley |
| A name of several methods is chosen by the type expected where it goes, and reported when nothing chooses | **decided**, Hadley |
| A bare method name inside its class is bound to `this`, and so inside a closure there | **decided while building it** |
| The call is virtual, as every call of a method is (record [0020](0020-the-base-chain-in-a-lookup.md)) | **decided while building it** |
| An overloaded **function** as a value -- a `def`, not a method -- is chosen the same way | **decided while building it** |
| As an argument, a name of several functions **waits** for its parameter, as a closure does | **decided while building it**, item 6's rule |
| A method of a value with no name -- `make().get` -- is refused | **decided while building it** |

## Before

`let f = c.add` passed the front end and g++ refused it -- the emitter wrote
`c.m_add_b6`, a member function where a value goes. A bare `add` inside the
class was refused by the emitter, *a method cannot be given as a value yet*.

## What it is

```haard
let f : (i32) -> i32 = c.add      # bound to c: f(5) adds to c.total
let g : (i32, i32) -> i32 = p.add # the other overload, through a pointer
let h : (i32) -> i32 = base.add   # Loud.add, if base points at a Loud
let k = b.get                     # one method: nothing to choose
apply(c.add, 2)                   # the parameter chooses
```

A function value is record 0058's pair, an environment pointer and a function
taking it first. A bound method's environment is the object's address -- or
the pointer itself, when the left is one -- and its function is an adapter,
one per method, that calls the method through it:

```cpp
static int32_t h0_1_Counter_m_add_b6_bound(void *self, int32_t a0) {
    return ((h0_1_Counter *) self)->m_add_b6(a0);
}
```

Since 'self' is the object's address and not a copy, `f(5)` changes `c`, and
a pointer to the base reaches the override.

**Choosing among several**: the candidate whose type is the expected one. A
method and the base's it overrides have one type and are one. Nothing
expected, or nothing of that type, is *'add' names 2 functions, so which one
has to be said by the type written where it goes*. As an argument, the name
is carried untyped (`ExpressionTyper::waits_as_a_function`, marked with
`Module::wait_as_function`), each candidate asks only whether its parameter is
a function, and the winner's parameter chooses -- the same order item 6 of
record [0067](0067-a-tuple-is-a-struct-the-compiler-writes.md) gave a tuple.

**Refused**: `make().get` -- the object would be gone by the end of the
statement and the value would refer to nothing at once. A local that goes
out of scope while the value lives on is record 0058's dangling, trusted.

Cases: `tests/emitter/cases/a_method_is_bound_to_its_object` (127, a bit per
rule) and `tests/type_table/cases/a_method_as_a_value_is_one_method`. Eight
sabotages, each caught.
