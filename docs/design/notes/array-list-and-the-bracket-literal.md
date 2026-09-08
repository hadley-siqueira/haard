# `Array<T>`, `List<T>` and `[1, 2, 3]` — the choices, for Hadley

Rewritten 2026-09-06 after Hadley settled four of the open questions. The first
draft proposed `[1,2,3]` as a hoisted local and a run of `add` calls; his
`List<i32>(i32*, i32)` is a better primitive and this note is rebuilt around
it.

## What is settled

1. **`[1, 2, 3]` infers as an `Array<T>`.**
2. **`[T]` is sugar for `List<T>`, and there is no native list type.** Record
   0022 stands.
3. **`List<T>` is a linked list**: nodes with `next` and `previous`, and `head`
   and `tail`.
4. **There is no literal for a `List<T>`.** One is always built from an
   `Array<T>`, from a constant-size vector written with `{}`, or from a pointer
   and a size passed explicitly.
5. **`let b : List<i32> = [1, 2, 3]` means
   `let b : List<i32> = List<i32>([1, 2, 3])`** — the compiler translates the
   binding into the **constructor call** it means.

## What that changes, and it is more than the answers

Point 5 says the compiler picks a constructor and writes the call. That is
**not** record 0018's coercion list growing an entry: a coercion is invisible
by the time the emitter runs, and `let hello : String = "hello"` works today
only because record 0026 emits `init` as a C++ constructor and **C++'s own
implicit conversion** does the rest.

Which is the same shape Hadley already ruled out for operators: *the emitter
must not lean on C++ having the feature*. So this is record 0034's mechanism
again — resolve it in Haard, write it down on the node, and let the emitter
write a call it can see.

**And point 4 collapses the whole design into one primitive.** Everything below
is `{...}` plus a `(T*, i32)` constructor:

| written | means |
|---|---|
| `{1, 2, 3}` | an `i32[3]`, a C++ array and nothing else |
| `[1, 2, 3]` | `Array<i32>(<that>, 3)` |
| `let b : List<i32> = [1, 2, 3]` | `List<i32>(<the Array>)` |
| `let b : List<i32> = {1, 2, 3}` | `List<i32>(<that>, 3)` |
| `let b : [i32] = [1, 2, 3]` | the same as two rows up — `[T]` is `List<T>` |

One `add` call per element becomes **one constructor call**, and the fixed
array literal — which the first draft treated as "next door, still open" — is
now the thing everything stands on.

## What is missing before any of it works

Three pieces, all small, and none of them is a decision:

- **`{1, 2, 3}` cannot be emitted.** It types as `i32[3]` and dies with *"this
  expression cannot be emitted yet"*.
- **`i32[3]` does not decay to `i32*`.** Measured: `takes(fixed, 3)` against
  `takes(i32*, i32)` is *no 'takes' takes these arguments*. Record 0018's list
  needs the entry, or the `(T*, i32)` route cannot be reached from a `{}` at
  all.
- **`Array<T>` needs `init(@from : T*, @count : i32)`**, the same one `List<T>`
  gets. It has neither today.

## 1. Where the fixed array lives

`{1, 2, 3}` is a C++ array, and a C++ array is a **declaration**, not an
expression. So the one real question is where the emitter puts it.

### 1a. A file-scope `static const` when every element is a literal, a hoisted local otherwise

```cpp
static const int32_t __fx0[3] = {1, 2, 3};   // [1, 2, 3]
h1_Array h0_2_xs(__fx0, 3);
```

**For it**: the common case — a literal written out — needs **no hoisting at
all**, so it works in a loop condition and on the right of `and`, which is
exactly where record 0032 has to refuse a template string. A constant array is
also emitted once for the whole program instead of rebuilt per turn of a loop.

"Constant" needs no constant evaluation: **every element is a literal token**
is a syntactic test, and agenda 5.3's evaluation is not required.

**Against it**: two paths for one construct, and a reader has to know which one
a given literal took.

### 1b. Always a hoisted local

```cpp
int32_t __fx0[3] = {1, 2, 3};
h1_Array h0_2_xs(__fx0, 3);
```

**For it**: one path. Reuses record 0032's hoisting and its three refusals
unchanged, so `f([1,2,3])` in a loop condition is refused with a sentence that
already exists.

**Against it**: `[1, 2, 3]` — a constant — cannot be written in a loop
condition or after `and`, which is a strange thing to refuse.

### 1c. Always a `static const`, and refuse a non-constant element

`[f(), g()]` is reported: *an array literal takes literals for now*.

**For it**: no hoisting anywhere, ever, and the smallest thing that runs.
Loosening it later is 1a.

**Against it**: `[x, y]` with two locals is an ordinary thing to write.

### Recommendation

**1c now, 1a next.** 1c is the smaller half of 1a and the half everything else
needs; it lands with a refusal in the shape record 0032 already uses, and 1a is
that refusal being lifted. Going to 1b instead means writing hoisting that 1a
would keep, but paying the refusal on constants forever.

## 2. How far "translate into a constructor call" reaches

The binding is settled. Four places take a value and three are not.

### 2a. A written type only: a binding and an assignment

`let b : List<i32> = [1,2,3]` and `b = [1,2,3]`. A call and a return are not
covered, so `f([1,2,3])` against `f(@l : List<i32>)` is *no 'f' takes these
arguments*.

**For it**: the narrowest reading of what was asked, and the one place where
the target type is unarguable — the author wrote it.

**Against it**: a parameter's type is written too, and a reader will not see
why one works and the other does not.

### 2b. All four places record 0018 already knows

A call, a return, a binding and an assignment — the same four the coercion list
reaches and the same four record 0031 asks its question at.

**For it**: one rule, in the place the language already keeps this kind of
rule. `f([1,2,3])` works.

**Against it**: at a **call** it interacts with overload resolution — `f(Array<i32>)`
and `f(List<i32>)` both match a bracket literal, and something has to rank
them. Record 0018's ranking has an answer (a step costs one), but it is a new
place for it to be wrong.

### 2c. 2b, and `char*` → `String` is migrated onto it

Record 0023's named entry in `Coercion` is deleted, and `String` gets the same
treatment as `List`: the compiler finds `init(@text : char*)` and writes the
call. The compiler stops knowing the name `String`.

**For it**: it removes a special case, and it stops the emitted C++ from
leaning on C++'s implicit conversion — the thing Hadley ruled out for
operators, applied to constructors.

**Against it**: it is a behaviour change to something that works, and it makes
**every** class with a one-argument `init` implicitly constructible, which is
C++'s converting constructor with all of its surprises. C++ has `explicit` for
exactly this and Haard would have nothing.

### Recommendation

**2b**, and **not** 2c yet. 2b answers "why does the parameter not work" before
anyone asks it, and its one risk — ranking at a call — is record 0018's
existing ranking doing its job.

2c is the right end state and the wrong next step: it turns a rule about
**written types** into a rule about **every class**, and that door does not
close again once programs depend on it. It deserves its own record, with the
question *does Haard need `explicit`* asked in it.

## 3. `List<T>` itself

A doubly linked list: a `Node<T>` with `value`, `next` and `previous`, and a
`List<T>` with `head`, `tail` and `size`. It owns its nodes, so record 0031
makes it write a `copy`, and record 0036's fix — a generic naming itself — is
what lets `copy` take a `List<T>&` at all.

Three constructors, and they are the whole of how one is built:

```haard
def init : void                    # empty
def init : void
    @from : Array<T>&              # what a bracket literal becomes
def init : void
    @from : T*
    @count : i32                   # what a '{}' becomes, and the explicit form
```

### `operator[]`

**Yes** — Hadley, 2026-09-06. It walks from `head`, so it is O(n) and a loop
over `l[i]` is O(n²); that is the author's to know and not the language's to
forbid. It gives back a `T&` like `Array`'s, so `l[2] = 9` works.

`operator==` too, over an ordinary `equals`, the way `Array` and `String` both
do it — an operator has no name, so neither can call the other.

### What else it declares

`length`, `is_empty`, `push_front`, `push_back`, `pop_front`, `pop_back`,
`first`, `last` — the operations a linked list is chosen **for**. Not `at`, not
`insert_at`: those are the ones an `Array` is for.

## 4. The order of work

Each line is usable on its own and nothing later changes anything earlier.

1. **`{1, 2, 3}` emits**, and `T[N]` decays to `T*` on record 0018's list.
   That is the primitive, and it is worth having whatever else is chosen.
2. **`Array<T>` gains `init(T*, i32)`**, and `[1, 2, 3]` becomes one call to
   it — section 1's choice decides where the array goes.
3. **The constructor call is chosen and written down**, section 2 — which is
   record 0034's mechanism with a constructor instead of an operator.
4. **`List<T>` is written**, in Haard, and needs nothing new from the compiler
   once 1 to 3 are in.

## What Hadley chose, 2026-09-06

| | chosen |
|---|---|
| **1. where `{}` lives** | **1a** — a file-scope `static` when every element is a literal, a hoisted local otherwise |
| **2. how far it reaches** | **2b** — a call, a return, a binding and an assignment |
| **the temporary** | **a named local**, so a literal that is not itself bound is hoisted |
| **the reach of the translation** | **only a `[]` or `{}` literal** — `let l : List<i32> = some_array` stays refused |
| **3. `List` indexing** | **`operator[]`, yes** |

A consequence of the named local worth writing down: it makes 1a and 1b
converge everywhere except a literal that **is** a binding's initialiser. A
literal passed where a **reference** is expected — which is most of them, since
`Array<T>&` is how one is passed — needs a name to bind to, and a name is a
statement. So record 0032's three refusals apply to a bracket literal too, and
what 1a keeps is that the **fixed array** underneath is written once at file
scope instead of rebuilt per turn of a loop.

### Built, 2026-09-06

Steps 1 and 2 of the order below. `{1, 2, 3}` emits, `T[N]` decays to `T*`,
`Array<T>` has `init(T*, i32)`, and `[1, 2, 3]` is one call to it — a static
array when constant, a local when not, hoisted into a binding when it is not
already one, and refused where record 0032 refuses.

Steps 3 and 4 — the constructor translation for a **written** class type, and
`List<T>` itself — are not built.

## Worked examples

### Point 1, the same source under all three

```haard
def contains : bool
    @xs : Array<i32>&
    @what : i32
    return false

def main : i32
    let n = 7

    let constant = [1, 2, 3]        # every element is a literal
    let computed = [n, n + 1]       # neither element is

    while contains([1, 2, 3], n):   # a constant one in a loop condition
        n = n - 1

    return 0
```

**1a — a static when constant, a hoisted local when not.**

```cpp
static int32_t __fx0[3] = {1, 2, 3};   // 'constant'
static int32_t __fx2[3] = {1, 2, 3};   // the one in the condition

int32_t h0_5_main() {
    int32_t h0_6_n = 7;
    h1_Array h0_7_constant(__fx0, 3);
    int32_t __fx1[2] = {h0_6_n, h0_6_n + 1};   // hoisted, it is not constant
    h1_Array h0_8_computed(__fx1, 2);
    while (h0_1_contains(h1_Array(__fx2, 3), h0_6_n)) {
        h0_6_n = h0_6_n - 1;
    }
    return 0;
}
```

Everything compiles. The one in the condition needed no hoisting because there
was nothing to hoist — the array is a static and the `Array` is a temporary.

Not `const`, because record 0029 leaves Haard without one and the constructor's
parameter is a plain `T*`. `Array` copies out of it, so nothing writes through
it in practice.

**1b — always a hoisted local.**

```cpp
int32_t h0_5_main() {
    int32_t h0_6_n = 7;
    int32_t __fx0[3] = {1, 2, 3};
    h1_Array h0_7_constant(__fx0, 3);
    int32_t __fx1[2] = {h0_6_n, h0_6_n + 1};
    h1_Array h0_8_computed(__fx1, 2);
    ...
```

and the loop is **refused**, although what it holds is three literals:

```
error: an array literal is built before the statement it is written in,
       so it cannot go in a loop condition
  --> main.hd:12:20
   |
12 |     while contains([1, 2, 3], n):
   |                    ^
```

**1c — always a static, and a non-constant element is refused.**

```cpp
static int32_t __fx0[3] = {1, 2, 3};
static int32_t __fx1[3] = {1, 2, 3};

int32_t h0_5_main() {
    int32_t h0_6_n = 7;
    h1_Array h0_7_constant(__fx0, 3);
    while (h0_1_contains(h1_Array(__fx1, 3), h0_6_n)) {
        h0_6_n = h0_6_n - 1;
    }
    return 0;
}
```

and `computed` is **refused**:

```
error: an array literal takes literals for now
  --> main.hd:9:20
   |
9 |     let computed = [n, n + 1]
  |                    ^
```

So 1c is 1a with the second half not written yet, and 1b is a different
first half that 1a would have to keep.

### Point 2, the four places

```haard
def takes : void
    @l : List<i32>
    pass

def gives : List<i32>
    return [1, 2, 3]                  # a return

def main : i32
    let a : List<i32> = [1, 2, 3]     # a binding
    let b : List<i32>

    b = [4, 5]                        # an assignment
    takes([6, 7])                     # a call

    return 0
```

**2a — a written type only** takes the binding and the assignment, and reports
the other two:

```
error: expected List<i32>, found Array<i32>
  --> main.hd:6:12
  |
6 |     return [1, 2, 3]
  |            ^
error: no 'takes' takes these arguments
  --> main.hd:14:5
   |
14 |     takes([6, 7])
   |     ^^^^^
```

**2b — all four** compiles the file. Every one of them becomes the constructor
call the compiler chose:

```cpp
static int32_t __fx0[3] = {1, 2, 3};
static int32_t __fx1[3] = {1, 2, 3};
static int32_t __fx2[2] = {4, 5};
static int32_t __fx3[2] = {6, 7};

h2_List h0_4_gives() {
    return h2_List(h1_Array(__fx0, 3));
}

int32_t h0_6_main() {
    h2_List h0_7_a(h1_Array(__fx1, 3));
    h2_List h0_8_b;
    h0_8_b = h2_List(h1_Array(__fx2, 2));
    h0_1_takes(h2_List(h1_Array(__fx3, 2)));
    return 0;
}
```

**Nothing there is a C++ implicit conversion.** Every constructor is named and
called, which is the operator rule applied to constructors.

### The ranking at a call, which is 2b's one risk

```haard
def show : void
    @xs : Array<i32>&
def show : void
    @l : List<i32>

show([1, 2, 3])
```

The literal **is** an `Array<i32>`, so the first is an exact match at cost 0
and the second costs one constructor step. Record 0018's ranking already says
the cheaper wins, so `show(Array<i32>&)` is chosen and nothing new was needed.

Two classes that both take an `Array<i32>` are a genuine tie and are reported
the way record 0012 already reports one:

```
error: this call matches more than one 'show' equally well
```

### The detail that only shows up in the C++

`h2_List(h1_Array(__fx0, 3))` binds a **temporary** `Array` to `List`'s
`@from : Array<T>&`, and C++ does not let a non-const reference bind to a
temporary. Three ways out, and they are worth choosing between:

- **The emitter writes a named local for the temporary.** That is hoisting,
  and it brings back everything 1a and 1c were avoiding.
- **The emitter writes `const_cast`**, which it already does for record 0031's
  copy constructor and for the same reason: Haard has no `const` to write on
  the parameter.
- **`let b : List<i32> = [1, 2, 3]` goes straight to `List(T*, i32)`** and
  never builds an `Array` at all:

  ```cpp
  h2_List h0_7_a(__fx1, 3);
  ```

  The bracket literal is *made of* a fixed array, so the compiler already has
  the pointer and the size. The `Array<T>&` constructor is then what an
  `Array` **variable** uses, not what a literal uses.

The third is smaller, faster and needs no `const_cast` — and it is a small
amendment to what was settled: `let b : List<i32> = [1,2,3]` means
`List<i32>(<the elements>)` and the compiler picks whichever of the three
constructors fits the **shape written**, rather than always going through an
`Array`.

## Still open next door

**A generic's body is checked with its parameters unbound**, so `left = 0` on a
`T` field is reported although every instantiation is fine. `Node<T>` will walk
straight into it — record 0036 names it and does not fix it.
