# 0072 — A bare name in a pattern is resolved

Status: **decided and built**, 2026-10-07. Hadley, after stage 7 of record
[0067](0067-a-tuple-is-a-struct-the-compiler-writes.md) was built with the
rule that a bare name in a tuple pattern **always captures**: a name that is a
variant of that element's enum is the variant, and any other name captures.
Reverses the rule stage 7 decided while building it.

| | |
|---|---|
| A bare name in a tuple pattern is a **variant** when the enum of its element declares one by that name | **decided**, Hadley |
| Any other bare name **captures**, by reference, as before | **decided**, Hadley |
| Which enum is asked is the **element's**, by its type -- not any enum in view | **decided**, Hadley |
| A name written twice as a variant, `(Off, Off)`, declares nothing and is no duplicate | **decided while building it** |
| In cases that share a block, a bare name is still taken as a capture and refused; write `Light.Off` there | **left as it was** |

```haard
switch t:                     # t : (i32, Maybe<i32>)
    case (0, Nothing):        # a literal and a variant
        ...
    case (n, Nothing):        # a capture and a variant; 'Nothing' in the body
        ...                   # is the variant too
    case (n, Just(v)):
        ...

switch u:                     # u : (Light, Power), both with an 'Off'
    case (Off, Off):          # Light.Off and Power.Off, by each element
        ...
    case (other, _):          # 'other' names no variant of Light: a capture
        ...
```

## Why it is decided in the type phase

Which enum an element is takes the subject's type, which the sugar pass that
takes the switch apart (record 0067, stage 7) does not have. And the switch
cannot be taken apart later instead: record 0040's lesson, a pass after the
type phase would reach the case's body with its names typed against nothing.

So the sugar pass still writes every bare name as a capture, `let n =
__sw[1]`, and marks its token with the flag its case raises
(`Module::mark_pattern_name`). When the type phase types that capture -- after
the element, before the body that reads it -- `TypeCollector::names_a_variant`
asks the element's enum for a variant by the name. If it has one,
`match_the_variant` rewrites the `let` in place:

```haard
__m0 = true
switch __sw0[1]:
    case Nothing:
        <the rest of the case>
    default:
        __m0 = false          # the next case is tried
```

and the capture's candidate is put out of view (`Module::unname`): the
NameResolver passes over it, so `Nothing` in the body finds the variant, and
the duplicate check does not count it. The statements that move into the case
keep their scopes, so what they resolve is unchanged. A clone of a generic
carries the marks on its tokens and is decided on its own.

Case: `tests/emitter/cases/a_bare_name_in_a_pattern_is_resolved` (31, a bit
per rule: a variant beside a literal and a capture, the variant named again
in the body, two enums with an `Off`, a name over an `i32` that captures,
a nested tuple, and a generic). Five sabotages, each caught; a sixth showed a
redundant test, which was removed.
