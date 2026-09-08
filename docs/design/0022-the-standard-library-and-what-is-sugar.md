# 0022 — Where `Array`, `List`, `String` and `Hash` come from, and what is sugar for what

Status: decided, 2026-09-02, by Hadley. Amends [0017](0017-the-prelude.md) and
closes most of [0021](0021-brackets-are-dynamic-braces-are-fixed.md).

## Context

Record 0017 decided *a* prelude and left four things open. Record 0021 found
three spellings of one idea and could not settle them without knowing what
`Array` is. Hadley settled both.

## Decision

**`Array`, `List`, `String` and `Hash` are ordinary classes from the standard
library.** Not builtins, not compiler magic: ordinary Haard, in `std.array`,
`std.string`, `std.list` and so on.

**The compiler is told where they are, and never searches.** The prelude is an
entry the roots-table generator **always writes**, which is record 0010's rule
applied to the one import nobody writes.

**The sugar, settled:**

| written | is | note |
|---|---|---|
| `[T]` | `List<T>` | sugar |
| `T[]` | `Array<T>` | sugar, as [0016](0016-the-type-table.md) already said |
| `{K: V}` | `Hash<K, V>` | sugar |
| `T[3]` | a fixed array of three | **not** a class, and the only array form that is not |

**The literals:**

| written | is |
|---|---|
| `[1, 2, 3]` | `Array<i32>` |
| `{1, 2, 3}` | `i32[3]` |
| `"abc"` | `char*` |
| a template string | `String` |

**A string literal is a `char*` first.** *"Primeiro tentar interpretar como
char\*. Se não for possível, virar String."* — Hadley, 2026-09-02. So it is
record 0018's first rule again: a literal takes what its context asks for, and
`char*` is what it is when nothing asks. A template string is always a
`String`, because it has to be built at run time.

**What "first" means is settled by
[0023](0023-a-char-pointer-becomes-a-string.md)**: `char*` → `String` is on
record 0018's coercion list, so a literal reaches a `String` parameter the same
way a variable does, and a literal matching the type it has when nothing asks
outranks one that had to take a step. With both `write(char*)` and
`write(String)` in view the `char*` wins, and it wins by the two lists rather
than by a rule written about strings.

`char` stays a builtin. `char*` is a pointer to one and needs nothing from the
standard library, which is why a plain string literal types **today**, before
any of this exists.

## Consequences

- **`TYPE_LIST` and `TYPE_HASH` do not survive**, and neither does a
  `TYPE_ARRAY` with no length. All four become a `TYPE_NAMED` pointing at a
  class of the standard library, once there is one to point at. `TYPE_ARRAY`
  **with** a length stays: a fixed array is an aggregate and not an object.
- **The standard library needs generics to work at all**, and generic
  substitution does not exist: `List<i32>.get()` gives back `T` today. So
  generics come **before** the prelude, which is the reverse of what the
  agenda assumed. `String` is the exception — it is not generic — and a
  standard library with only `String` in it would unblock template strings
  without generics.
- **`{1,2,3}` is the one container literal that needs nothing.** It is a fixed
  array, it is not a class, and it works today.

## Open inside this

1. **One implicit import or several.** The compiler could be told one module
   name — `std.prelude`, say — which itself imports `std.array`, `std.string`
   and the rest and is ordinary Haard the library's author controls. That keeps
   record 0008's "nothing is implicit" at exactly **one** exception instead of
   four, and keeps the compiler from knowing the library's shape. Proposed, not
   decided.
2. **Whether the module may be named.** `prelude::Array` to disambiguate from
   an `Array` of your own. Record 0017's second open end, still open.
3. **`hdc file.hd` with no table.** It compiles one file today and follows no
   imports. With the prelude coming from the table, that mode has no `String`
   and no `Array` at all. Either it keeps working with less, or it stops being
   a mode.
4. **The literal and the type syntax disagree.** `[1, 2, 3]` is an `Array<i32>`
   while `[T]` is a `List<T>`. Reading them together the way record 0018 reads
   a literal — *the context decides, and `Array` is what it is when nothing
   asks* — makes `let a : [i32] = [1, 2, 3]` a `List<i32>` and `let a =
   [1, 2, 3]` an `Array<i32>`. That is what the typer already does with the
   element type, and it is the reading assumed here. **Not confirmed.**
5. **A symbol literal** still has no type and no record says what one is.
