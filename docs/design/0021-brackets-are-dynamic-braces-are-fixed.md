# 0021 — `[]` is a dynamic array, `{}` is a fixed one

Status: decided, 2026-09-02, by Hadley. What it leaves open is named at the end
and is the reason this record exists.

## Decision

Hadley, 2026-09-02:

**`[1, 2, 3]` is a dynamic array** — an `Array<i32>`, the growable thing the
prelude declares (record 0017).

**`{1, 2, 3}` is a fixed-size array** — an `i32[3]`, its length written by how
many elements were written.

Two brackets, two different things, and which one you get is decided by the
punctuation and never by inference.

## What was already right

The parser has told the two apart from the beginning: `parse_list` builds an
`AST_LIST` for `[...]` and `parse_array_or_hash` builds an `AST_ARRAY` for
`{...}`. When the typer learned to type them on 2026-09-02 it gave

- `{1, 2, 3}` a `TYPE_ARRAY` with a length of 3, which is exactly right
- `[1, 2, 3]` a `TYPE_LIST`, which is the right **shape** — a sequence of one
  element type with no length — and the wrong **name**

So nothing in the typer is wrong. What is provisional is that the dynamic one
is a `TYPE_LIST` rather than an `Array<i32>`, and it is provisional for the
same reason everything else about `Array` is: **the prelude does not exist
yet**, so there is no `Array` to name.

## What this opens, and it is the point of the record

There are now **three spellings of one idea**, and nothing says whether they
are one idea:

| written | type today | record |
|---|---|---|
| `T[]` | `TYPE_ARRAY` with no length | [0016](0016-the-type-table.md) says it is sugar for `Array<T>` |
| `[T]` | `TYPE_LIST` | nothing has said what it is |
| `Array<T>` | does not exist | [0017](0017-the-prelude.md) will declare it |

If `[T]` is a third way to write `Array<T>`, then `TYPE_LIST` should not
survive the prelude: it collapses into a `TYPE_NAMED` pointing at the prelude's
class, the same way `T[]` is meant to. Two of the three spellings would then be
sugar and the language would have one growable sequence with three faces —
which is a lot of faces.

If `[T]` is something else — a linked list, a slice, a view — then it is a
second container and needs a record of its own saying what it is and what it
costs.

**Not decided.** It cannot be decided usefully before the prelude, because
what `Array` *is* is the other half of the question. Whoever writes record 0017's
open ends writes this one with it.

## Consequences

- **`tests/type_table/cases/every_expression_kind` shows both**, `[i32]` next
  to `i32[3]`, so the day one of them changes it is a golden that moves.
- **Nothing is being renamed now.** Calling `TYPE_LIST` something else before
  knowing whether it survives would be churn, and record 0016's interning does
  not care what the kind is called.
