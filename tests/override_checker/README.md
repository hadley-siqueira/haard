# OverrideChecker tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project. The golden is the diagnostics, and a
case whose golden is `no diagnostics` is a case that says a correct program
stays quiet.

## The rule

Record 0020 makes a method with the same parameters as one of its bases an
**override** of it, and keeps the return type out of that decision — because
record 0012 keeps the return out of what tells two overloads apart. Consistent,
and it left the return unchecked entirely.

This phase checks it, with C++'s rule: the override's return must be **the same
type, or a pointer or reference to something derived from what the base gives
back**.

The question the rule answers is always the same one. Somebody holding a
`Shape*` calls `clone()` without knowing a `Square` is underneath. **Is what
comes back still usable by someone who expected the base's answer?**

- `Square*` where `Shape*` was promised — yes. Every `Square*` is a `Shape*`.
- `f64` where `i32` was promised — no. Not a wider integer either: `i64` is not
  an `i32`.

## What each case pins

| case | what it proves |
|---|---|
| `a_covariant_return_is_allowed` | `Derived*` over `Base*`, `Derived&` over `Base&`, and an identical return — all quiet |
| `a_different_return_is_reported` | `f64` over `i32`, and `i64` over `i32`. A wider integer is not covariance |
| `covariance_is_not_for_a_value` | `Derived` over `Base` **by value** is rejected |
| `covariance_only_goes_down` | `Base*` over `Derived*` is rejected — the caller was promised the derived one |
| `a_pointer_does_not_override_a_reference` | covariance is between two pointers or two references, never across the two |
| `a_different_signature_is_not_an_override` | different parameters are two real overloads, and their returns have nothing to do with each other |
| `an_override_reaches_past_a_middle_class` | a class in between that writes no such method is walked through, and the override is found two steps up |

## Why a value cannot be covariant

Size. A caller going through the base reserved room for a `Base`, and a
`Derived` does not fit — it would be sliced. Two pointers are the same size
whatever they point at, which is exactly what makes those safe. C++ rejects the
same thing for the same reason:

```
error: invalid covariant return type for 'virtual Derived Square::make()'
```

## Two things the sabotages taught

**The pointer-or-reference guard needed a case across the two kinds to be
provable.** Removing it and testing only `Derived` over `Base` by value still
reported, by accident: a named type has no arguments, so the covariance walk
read a sentinel and answered no. It took `Derived*` over `Base&` for the guard
to be the thing that decided.

**A guard against an unbuildable type was dead code and was deleted.** Record
0016 poisons a whole signature when any part will not build, and both callers
already skip a candidate whose type is poisoned — so nothing could ever reach
it. Its sabotage failing zero cases is what said so.

## Where the diagnostic points

At the method's **name**, not at the written return type. A composite type node
carries no token of its own — `AstBuilder::make_named_type` builds one with
token 0 and lets the name below it hold the span — so reporting at a return
type lands on the first token of the file. The message names both types anyway;
what the span has to say is *which method*.
