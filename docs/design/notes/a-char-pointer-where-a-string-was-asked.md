# A `char*` where a `String` was asked for

Written 2026-09-02, at Hadley's request, to be picked up next session. Nothing
here was decided when it was written.

**Answered on 2026-09-03 by [record 0023](../0023-a-char-pointer-becomes-a-string.md),
and this note is kept for the framing and not for the answer.** In short: a
`String` **owns** its bytes, so a literal reaching one runs a constructor and
question 1 was never rule 1; `char*` → `String` went on record 0018's list, so
questions 1 and 3 have the same answer and the literal needs no case of its
own; and question 2 is a literal's distance from the type it has when nothing
asks, which is rule 5's integer again rather than an exception to it.

What the note missed, and what decided the rest: **whether rule 1 is free
depends on what a `String` is**, and no record had said.

## The question

```
def puts : void
    @s : String

puts("abc")     # the literal is a char* by default (record 0022)
```

Record 0018 has **no implicit conversion**, and its coercion list is closed. So
a `char*` does not become a `String` and this call matches nothing.

## It is really three questions, and only the first is easy

### 1. The literal — already answered, mechanically

Hadley, 2026-09-02: *"first try to read it as a `char*`. If that is not
possible, make it a String."*

That is record 0018's **first rule**, the one that already makes `f(3)` work
against a `u8` parameter: a literal has no type of its own and takes what the
parameter asks for. The machinery exists — `ExpressionTyper::call` carries a
literal into the call untyped and `OverloadResolver::match` asks each candidate
whether it fits.

What it needs: `AST_STRING_LITERAL` marked as a literal at the call, and
`match` extended so a string literal answers to a `char*` parameter **and** to
a `String` one.

### 2. Which one wins when both exist

```
def write : void
    @s : char*
def write : void
    @s : String

write("abc")
```

Under rule 1 alone both match with a score of 0, which is an **ambiguous call**
— record 0012's rule, reported at the call. Hadley's phrasing says otherwise:
*char\* first*. So a string literal matching `char*` has to **score better**
than one matching `String`.

Record 0018's scoring has no notion of one literal match being better than
another — every literal match is 0 today. This would be the first exception,
and it is small, but it is a change to the ranking and not to the list.

### 3. The variable — the one that needs a real decision

```
let s = "abc"    # a char*, and no longer a literal
puts(s)          # the parameter is String
```

There is no literal here. `s` is a genuine `char*`, and record 0018 says
nothing converts. Three ways out, and they are not equal:

**(a) Leave it failing.** The programmer writes `String(s)` or `s.to_string()`.
Honest, costs nothing, and consistent with everything decided so far. The
annoyance is that it makes `String` feel second-class next to `char*`.

**(b) Add `char*` -> `String` to record 0018's coercion list.** One entry, the
same shape as the three already there. But every entry so far is a *language*
relation — a literal, a derived class, a pointer read through a dot. This one
would be a **library** relation: it names a class the compiler does not know
about, and it needs a constructor to run, which is code. Record 0018 chose a
closed list precisely to avoid that.

**(c) Converting constructors, C++ style** — any `init(T)` makes `T` implicitly
convertible. That is the general form of (b), and it is the thing record 0018
rejected wholesale. It would reopen the record.

**Leaning:** (a) plus rule 1. A literal reaching either parameter covers what
people actually write, and a real `char*` variable becoming a `String` is a
copy and an allocation that is better written down.

## What it touches

- **Record 0018** if (b) or (c). Rule 1 alone touches nothing — it is 0018
  applied to one more literal kind.
- **Overload ranking** for question 2, in `OverloadResolver::match`.
- **Record 0022**, which said the literal is a `char*` "first" and left what
  "first" means to this note.
- **`String` has to exist**, so none of it can be tested before the standard
  library does — which means after generics, except that `String` is not
  generic and could come alone.
