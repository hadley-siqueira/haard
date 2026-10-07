# 0067 — A tuple is a struct the compiler writes

Status: **decided** 2026-10-07, Hadley, over three rounds in one conversation
that compared Rust, Swift, C#, Scala, Python, TypeScript, Nim, D, Zig, C++,
Kotlin, Go, Lua, Haskell and OCaml. **Built** the same day, in seven stages -- see the end. It makes emittable what
record [0016](0016-the-type-table.md) reserved (`TYPE_TUPLE`), and the parser
already reads `(i32, bool)`, `(1, 2)` and `let (a, b) = t`.

| | |
|---|---|
| A tuple is a **struct the compiler generates, one per shape**; `(i32, bool)` is the same type in every module | **decided**, Hadley |
| `t[0]` reads a field; it is **never** a call to `operator[]`; the index is known at compile time, `t[i]` and an index out of range are errors | **decided**, Hadley |
| A tuple may hold a `T&`; the literal `(x, y)` holds **copies**, references only where the type is written | **decided**, Hadley |
| Destructuring writes its **parentheses**, copies unless `T&` is written, takes `_` and nests; there is **no rest** | **decided**, Hadley |
| A tuple can be **assigned to**: `(a, b) = (b, a)`, the right side whole before any write | **decided**, Hadley |
| A new name in that assignment **declares** it, as record [0027](0027-a-bare-name-that-declares.md) says of `x = 1` | **decided**, Hadley |
| `switch` takes a tuple, with literals, `_`, captures and nested enum variants as patterns | **decided**, Hadley |
| A `switch` over a tuple that covers nothing and writes no `default` **falls through silently** | **decided**, Hadley, record [0047](0047-what-the-language-expects-of-the-programmer.md) |
| In a tuple pattern a **bare name always captures**; a variant is written as a call, `Some(y)`, or with its enum, `Option.None` | **decided while building it** -- for Hadley to confirm |
| `for (k, v) in c` writes its parentheses, and the names are **always references** | **decided**, Hadley |
| A Hash is Python's: `for k in h` gives keys, `h.items()` gives `(K&, V&)`, and `for (k, v) in h` calls `items()` **for** the programmer | **decided**, Hadley |
| That rule holds for **any class** with an `items()` | **decided**, Hadley |
| A parameter may be a pattern, in a `def` and in a closure | **decided**, Hadley |
| `==` and `!=` element by element, and a `to_string` the compiler writes | **decided**, Hadley |
| No `()` and no tuple of one; `(x)` is parentheses; a multiple return is `return (a, b)` | **decided**, Hadley |
| `hash_of` of a tuple, and so a tuple as a Hash key | **later** |
| A key changed through its `K&` corrupts the hash, and is not refused | **decided**, record [0047](0047-what-the-language-expects-of-the-programmer.md) |

## The syntax

```haard
def min_max : (i32, i32)
    @xs : i32[]

    ...
    return (low, high)            # the parentheses are required

def main : i32
    let t = (1, "one")            # (i32, String), both copies
    let n = t[0]                  # the field, read directly
    let s = t[1]

    let (a, b) = t                # a and b are copies
    let (x, _) = t                # _ takes nothing
    let (p, (q, r)) = (1, (2, 3)) # nested

    let u : (i32&, i32&) = (x, a) # references, because the type says so
    u = (5, 6)                    # writes x and a, through the references

    (a, b) = (b, a)               # a swap: the right side is built first
    (c, d) = min_max(xs)          # c and d are new names, so declared here

    for (k, v) in ages:           # ages.items(), written by the compiler
        v += 1                    # changes the hash

    switch t
        case (0, _)
            println("zero")
        case (n, s)
            println("${n}: ${s}")

    println("${t}")               # (1, one)
    return 0
```

## Why `t[0]` and not `t.0`

The programmer never sees the generated struct, so its fields have no names
anyone could write; `t[0]` is the spelling for *the first field*, the way Zig,
Nim and D read a tuple. It is a **special case of `[]`**, decided in the type
phase by the subject's type: on a tuple it is sugar for the field and nothing
is called. So the index must be known when compiling -- today that means a
literal, since `const` is a convention ([0029](0029-const-is-a-convention-for-now.md));
a `const` name joins when `const` is real -- and `t[i]` is an error, as is an
index past the last element.

`t.0` was the other candidate (Rust, Swift). Besides being a field nobody can
name, it has the trap Rust had: the scanner reads `t.0.1` as `t` `.` `0.1`.

## References

A literal holds copies: `(x, y)` is `(i32, i32)`. A tuple of references exists
only where its type is written, `(i32&, i32&)`, and then its elements are
references in record [0035](0035-a-reference-is-the-thing-it-names.md)'s
sense -- **assigning to the tuple writes through every one of them**, which is
the same thing `(x, a) = (5, 6)` does.

## Destructuring

The parentheses are always written: `let (a, b)`, `(a, b) = ...`,
`for (k, v) in`, `@(x, y) : (f64, f64)`, `|(x, y)| {...}`. A pattern is a
name, `_`, or a parenthesised list of patterns; nothing else, and no rest.

| where | a name is |
|---|---|
| `let` | a **copy**, record [0062](0062-a-binding-is-a-copy-of-what-it-is-given.md), unless the type says `T&` |
| an assignment | a place written to; a name not in view is **declared** (record 0027) |
| `for` | a **reference**, always, in any container (record [0040](0040-for-in-is-an-iterator.md)) |
| a parameter | what the parameter's type says |
| a `case` | a capture, a **reference** (record [0043](0043-an-enum-is-a-tagged-union.md)) |

An assignment builds its whole right side before writing anything, so
`(a, b) = (b, a)` swaps. Its targets are any place (`xs[i]`, `p.x`) or `_`.

## `for` over a class with `items()`

For `for (pattern) in c`:

1. if `c.iterator()`'s cursor gives a tuple the pattern fits, that is walked;
2. else, if `c.items()` exists and its cursor gives a tuple the pattern fits,
   the compiler writes `c.items()`;
3. else it is an error that says what `c` gives.

`for k in h` is untouched -- record [0042](0042-the-hash-and-what-a-generic-may-ask.md)'s
keys -- and `for p in h.items()` names the pair whole. The rule finds `items`
by name the way record 0040 finds `iterator`, so a class of the programmer's
gets it too. `Hash` gains `items()`, whose cursor gives `(K&, V&)`.

## `switch`

A `case` of a tuple is a pattern: a literal, `_`, a name that captures, a
nested tuple, or an enum variant with its own captures, as record 0043 has
them. A `switch` over a tuple **need not be exhaustive**: a value no `case` matches
and no `default` takes runs nothing, and execution goes on after the `switch`.
Hadley, 2026-10-07: *"confiamos no programador. Se não tiver default e não for
coberto todos os casos, apenas passa silenciosamente e continua a execução.
[...] o objetivo da linguagem é fornecer ferramentas para o programador usar e
não ter que ficar verificando a cada passo se ele esqueceu de algo"*. No
warning either, for now: one would need the coverage analysis this rule exists
to avoid.

## What the compiler writes for a tuple

A C++ struct per shape, emitted once, with one field per element; its copy,
its destroy and its move go element by element, by each element's own
([0031](0031-what-copying-a-value-means.md), [0066](0066-a-move-is-written.md)).
`==` and `!=` compare in order with each element's own `==`. `to_string`
writes `(a, b)` with each element's `${}`, so `${t}` works when every element
does, and is an error naming the one that does not.

## Rejected

- **`Tuple2<A, B>` in the standard library**: one class per arity, since there
  are no variadic generics.
- **`t.0`**, above. **A runtime `t[i]`**: an element's type would be a union.
- **`for k, v` without parentheses** (Python, Go): one idea would have two
  spellings beside `let (a, b)`.
- **The Hash iterating pairs** (Rust): `for k in h` would change meaning with
  no error to say so.
- **A rest pattern**: it only means something with dynamic or variadic tuples.

## Built so far

**Stage 1, 2026-10-07**: the struct, the literal, `return (a, b)`, `t[n]` and
its two refusals, `(T)` as a group, `(a,)` refused, a postfix on a tuple type,
and assignment element by element.

- `ExpressionTyper::tuple` gives each element what the shape wanted, by
  `Coercion::fits` -- `(text, 1)` with a `char*` text is a `(Name, i32)` when
  that is asked for -- and takes the value of a reference otherwise.
  `ExpressionTyper::tuple_element` is `t[n]`; `TypeBuilder` reads `(T)` as T.
- The emitter declares every concrete tuple in any module's table after the
  forward declarations, and `emit_tuple` writes each in section two after what
  it holds (`complete`), so a class holding one and one holding a class both
  come out in order. `declares_copy` says yes for a tuple with an element that
  asks for `m_assign`, or one that is a reference, and the tuple then writes
  its own `m_assign`, plus one taking a temporary.
- The parser takes `postfix*` after a tuple type, in `hdc` and in the
  bootstrap.

Cases: `tests/parser/cases/type_tuple_takes_a_postfix.hd`,
`tests/type_table/cases/a_tuple_is_read_at_a_written_position`,
`tests/emitter/cases/a_tuple_is_a_struct_per_shape` (190: two modules, a class
field, a generic, nesting, a Box that owns a pointer, a converted element) and
`tests/programs/cases/a_tuple_holds_what_owns_memory` (String elements, clean
under the address sanitizer). Twelve sabotages, each caught; one first looked
uncaught because a literal is already a construction when it reaches the
emitter, which is what made the case use a `char*` value -- and that found the
missing coercion above.

**Stage 2, 2026-10-07**: `==` and `!=`, `${t}`, references, and moves.

- `==` is typed by `ExpressionTyper::binary`: a tuple has only `==` and `!=`,
  the two sides are the same **value shape** (`value_shape`: every reference
  read as what it names, so `refs == (1, 2)` works), and every element has to
  compare (`compares`). The emitter writes it **where it is used**, one
  comparison per path down to a non-tuple (`l.e1.e0 == r.e1.e0`), inside a
  C++ lambda whose two parameters read each side once. An `__equals` per shape
  was the first version; it put a method in every struct for the few that are
  compared, and could not compare two shapes of one value.
- `${t}` is taken apart by the `StatementChecker`, which is the first thing to
  type the statement `__tsN.append(t)`: it types the piece alone, and a tuple
  becomes the appends of `(`, each element, `, ` and `)`, with anything that
  is not a name bound to `__tvN` first so a call runs once. A nested tuple
  comes back through the same door. It is there and not in a pass of its own
  because the logger keeps every report, so typing the piece in one phase and
  the call in another would report a mistake in it twice. `t.to_string()`
  written by hand does not exist: only `${}` writes a tuple.
- A literal element is a reference only when the shape asks for one **and**
  the element is a place, so `(x, a)` into `(i32&, i32&)` refers and `(5, 6)`
  stays two values.
- An assignment between two tuples of one length whose values differ --
  `(char*, i32)` into `(String, i32)` -- and **every** assignment to a tuple
  holding references is split by the checker into one assignment per element,
  from a copy of the right side made first, so `swap = (q, p)` swaps. The right
  side of such an assignment is typed as the value shape.
- A tuple holding something with a move `init` gets a static `__move`, which
  builds a new one moving that and copying the rest, and an `m_move_assign`.
  A constructor would have made the struct stop being an aggregate.

Cases: `tests/type_table/cases/a_tuple_compares_and_nothing_else`,
`tests/emitter/cases/a_tuple_is_compared_referred_and_moved` (127, a bit per
behaviour), and five checks more in
`tests/programs/cases/a_tuple_holds_what_owns_memory` -- among them a call in
`${}` that has to run once. Fourteen sabotages, each caught.

**Stages 3 and 4, 2026-10-07**: `let (a, b) = t` and `(a, b) = (b, a)`, both
taken apart by the `SugarLowerer`, before any name is collected -- they need
no type, only the shape that was written.

- `let (a, (b, _)) : T = t` is a `let` per name, `let a = t[0]`,
  `let b = t[1][0]`, so each is a copy (record 0062). A type written as a
  tuple gives each name its own element's type, so a name written `String&`
  refers into `t`; a type of another length is refused. What is not a name is
  bound to `__dN` first, with the whole type, so a call runs once. `_` binds
  nothing, `(a)` is a in brackets, and `const` stays `const`. The parser reads
  a pattern inside a pattern now, in `hdc` and in the bootstrap.
- `(a, b) = e` is `let __dN = e` -- always, so `(a, b) = (b, a)` swaps -- and
  then `a = __dN[0]`, `b = __dN[1]`. A target is any place, a pattern or `_`.
  A name nobody declared is declared by its own assignment, record 0027, for
  free: the `ImplicitCollector` runs after this pass and sees `a = __d0[0]`.

Cases: `tests/parser/cases/binding_target_nests.hd`,
`tests/sugar/cases/a_tuple_is_taken_apart_into_names`,
`tests/sugar/cases/a_tuple_is_assigned_apart`,
`tests/type_table/cases/a_tuple_taken_apart_must_fit`, and seven checks more in
the whole-program case, among them a swap of two Strings and a reference into
a tuple. Twelve sabotages, each caught.

**Stage 5, 2026-10-07**: `for (k, v) in c`.

- The sugar pass makes it `for __eN in c:` with a `let` per name at the top of
  the body, out of `__eN` -- `destructure_into` again. Each of those names is
  marked on its token (`Module::bind_by_reference`), and the type phase makes
  a marked name a **reference**, and one to the place it is given when that
  is a field, `__e0[1]`. By token and not by node, because a generic is cloned
  node by node into the same module and the clone carries the tokens.
- The pattern's length is kept on the `for` token too, and the
  `ForEachLowerer` reads it: when what `c.iterator()` gives is not a tuple of
  that length (`gives_a_tuple`, asked on a clone of `c` and only through
  members the class has, so asking reports nothing) and `c` has an `items`,
  the loop walks `c.items()`. Any class.
- `Hash` has `items()` now, a `HashItems<K, V>` whose cursor gives
  `(K&, V&)`, and `value_at` beside `key_at`.
- `for k, v in` with no brackets is still refused, and the message says how to
  write it.

**Found on the way, and fixed**: a loop variable over a **fixed array** was a
copy. Record 0040 says it is a reference, and over a class it is, because a
cursor's `next` gives a `T&`; over an `i32[3]` it is `xs[__i0]`, whose type is
the element's own, so keeping the reference kept nothing and `x += 10` changed
nothing, in silence, since 2026-09-08. `tests/type_table/cases/a_loop_variable_is_what_it_walks`
had pinned the copy (`f  i32`) under a comment that said otherwise.

Cases: `tests/sugar/cases/a_loop_takes_a_tuple_apart`,
`tests/emitter/cases/a_loop_takes_a_tuple_apart` (31: a class whose
`iterator` gives numbers and whose `items` gives pairs, one whose `iterator`
already gives pairs, a fixed array nested and with `_`, and the plain fixed
array), and two checks more in the whole-program case, a Hash and an Array of
Strings. Six sabotages, each caught.

**Stage 6, 2026-10-07**: a parameter that is a pattern, in a `def` and in a
closure.

- The parser reads a pattern where a parameter's name goes, `@(x, y)` and
  `|(x, y)|`, in `hdc` and in the bootstrap. Two parser cases had pinned the
  opposite, `binding_target_is_not_a_param` and `..._closure_param`; they say
  the new rule now, under the names `binding_target_is_a_param` and
  `binding_target_is_a_closure_param`.
- The sugar pass gives the parameter a name of its own, `__pN`, and starts the
  body with a `let` per name out of it -- `destructure_into` a third time.
  With a type written, each name takes its element's; a type written as a
  reference to a tuple, `(A, B)&`, makes each name a reference to its element,
  which holds for `let (a, b) : (A, B)& = t` too. With no type -- a closure
  typed by where it goes -- each name is marked to refer, as in a `for`, so
  `ages.items().each(|(k, v)| { v += 1 })` writes the Hash.
- `HashItems` has `each`, taking `((K&, V&)) -> void`: a function type's
  bracketed list is its parameters (record 0058), so one tuple parameter is
  written in a second pair of brackets.

**Found on the way**: a closure called with a tuple literal of references,
`f((key_at(i), value_at(i)))`, gets copies -- an argument to a function value
is typed before the parameter is known. `HashItems.each` walks its cursor
instead, and the general case is left open. And the first version of
`take_parameter_apart` wrote through an `AstNode*` taken before a node was
made: one program in four lost its parameter, depending on where the node
vector grew. The rest of the compiler was searched for the shape and has none.

Cases: `tests/parser/cases/binding_target_is_a_param.hd`,
`tests/parser/cases/binding_target_is_a_closure_param.hd`,
`tests/sugar/cases/a_parameter_takes_a_tuple_apart`,
`tests/emitter/cases/a_parameter_takes_a_tuple_apart` (15), and three checks
more in the whole-program case -- `each`, `map` and a Hash's `each`. Five
sabotages, each caught.

**Stage 7, 2026-10-07**: `switch` over a tuple, taken apart by the sugar pass
whenever a case is written as a tuple.

- It is a chain of `if`s on a flag, `__mN`: each asks `not __mN` and its own
  tests, raises the flag, binds its captures and runs its block, and `default`
  runs when the flag is still down. A flag and not an `elif` chain, because a
  case can fail **inside**: an element written as a call, `Circle(r)`, is a
  variant, matched by a `switch` of its own around the rest of the case --
  whose `default` does nothing, leaving the flag down for the next case.
- A literal or any other value is compared with `==`, so a String element by
  its own; `_` tests nothing; a pattern nests; cases with no block share the
  next one's, joined by `or` in brackets; and cases that share a block may not
  capture or match a variant with a payload, since which one matched is not
  known (record 0043's rule for groups, in its tuple form).
- A capture is a `let` out of the subject, marked to refer, as record 0043's
  captures are references. The subject is read through its name, or bound to
  `__swN` when it is not one.
- A value no case matches, with no `default`, runs nothing, which is what
  Hadley asked for. Nothing is reported, and no warning either: one would need
  the coverage analysis that rule exists to avoid.

**Decided while building it**, and **reversed** by Hadley the same day in
record [0072](0072-a-bare-name-in-a-pattern-is-resolved.md) -- a bare name is
the variant when its element's enum has one by that name: a bare name inside
a tuple pattern **always captures**. `case (n, None)` would bind a capture called `None`; the variant
is written `Option.None`, or as a call when it carries something. Telling a
capture from a variant by name needs the names resolved, which is after the
sugar pass -- and a capture has to be declared before the type phase infers
the bindings of the case's block, or they are typed against nothing. Rust has
the same rule and the same trap, and lints it.

Cases: `tests/sugar/cases/a_switch_takes_a_tuple_apart`,
`tests/emitter/cases/a_switch_takes_a_tuple_apart` (127, a bit per rule), and
one check more in the whole-program case. Eight sabotages, each caught.

## A tuple literal written as an argument, 2026-10-07

Decided by Hadley after the stages above, as item 6, and built the same day.
`bump((a, b))` with `bump` taking an `(i32&, i32&)` was refused -- *no 'bump'
takes these arguments* -- although `let p : (i32&, i32&) = (a, b)` refers to
a and b. A call typed every argument before choosing among its candidates, and
a tuple with nothing asked of it holds copies. Not a question of semantics but
of order: the binding knows its destination before it types the tuple, and the
call did not.

**The rule**: at a call a tuple literal is a literal, as `200` is (record
0018). It is carried untyped, and each candidate asks it to be its parameter
element by element: a number to be its element, `null` a pointer, a closure a
function of as many parameters, a tuple inside the same way down, and anything
else -- typed once, before the candidates, since it has a type of its own -- by
the list every argument answers to. Once one wins the tuple is built against
it, by the binding's own rule: a reference where the parameter writes one and
the element is a place, and the tuple built is checked against the
parameter. (`bump((a, 2))` was refused for its 2 until record
[0068](0068-a-reference-takes-any-value.md), the same day, gave a `T&` any
value.)

```haard
bump((a, b))             # refers to a and b
pick((200, null))        # a u8 and a pointer, asked by the parameter
over((a, b))             # the overload taking (i32&, i32&)
over((a, 1.5))           # the one taking (i32, f64)
f((a, b))                # f a value of type ((i32&, i32&)) -> void
Box((7, 8))              # an 'init' is asked the same way
```

**Why**: Hadley, shown how C++ does it -- `bump({a, b})` into a
`std::tuple<int&, int&>` refers to a and b, a braced list is asked by each
overload, and `std::make_tuple(a, b)` fails exactly as Haard did -- and how
Rust (the reference written at the call), C#, Swift and Zig (a literal typed
by its destination, with no reference to take) do. The reference coming from
the destination type is records 0035's and this one's model, and he chose
**not** to reopen it. Typing the tuple by the parameter only when there is one
candidate was refused for breaking at a distance: a second overload written
elsewhere would make a call stop compiling.

**Built**: `ExpressionTyper::tuple_argument` types what has a type of its own
and gives the tuple's type with nothing asked of it, which is what a generic
is solved from; `OverloadResolver::tuple_match` asks the shape;
`ExpressionTyper::tuple_given` builds and checks the winner's, at a call, at a
construction and at a call of a function value.

Cases: `tests/emitter/cases/a_tuple_literal_takes_its_parameters_shape` (255,
a bit per rule), `tests/type_table/cases/a_tuple_literal_argument_must_fit`.
Eight sabotages, each caught.

**Found on the way**, and closed the same day:

- A bare variant of a generic enum was not asked by its parameter at a call
  at all, tuple or not -- `take(Option.None)`, with `take` taking an
  `Option<i32>`, said nothing says what `T` is, though `let o : Option<i32> =
  None` worked. Now `None` and `Option.None` naming one variant of a generic
  nobody instantiated **wait** for the parameter, as a number does
  (`ExpressionTyper::waits_as_a_variant`, marked on the node with
  `Module::wait_as_variant`): each candidate asks whether its parameter is a
  clone of that enum, and the winner types it. Two candidates that both are
  make an ambiguous call. Cases:
  `tests/emitter/cases/a_bare_variant_waits_for_its_parameter` (31),
  `tests/type_table/cases/a_bare_variant_needs_one_parameter`.
- A generic was not solved through a tuple: record 0059's `unify` and
  `substitute` had no tuple, so `first((a, 1))` and `first(t)`, with
  `first<T>` taking a `(T, i32)`, both said nothing says what `T` is. A tuple
  is now laid over a tuple of its own length, element by element. A closure
  **inside** a tuple literal still says nothing about a generic, since the
  tuple has no type of its own while the closure waits. Case:
  `tests/emitter/cases/a_generic_is_solved_through_a_tuple` (15).

Eight sabotages more, each caught.

## What is left open

- `t.to_string()` written by hand: a tuple is written into a String only by
  `${}`.
- `hash_of` of a tuple, so a tuple as a Hash key.
- ~~An argument to a function value is typed before the parameter is
  known~~ -- a tuple literal, and a bare variant of a generic enum, are asked
  by their parameter since 2026-10-07 (the section above).
- A closure inside a tuple literal does not help solve a generic:
  `apply((|x| -> i32 { ... }, 4))` with `apply<T>` taking a
  `((T) -> T, T)` has to write `apply<i32>`.
- A generic body is typed with its parameters unbound, and two generics'
  `K`s are different types there, which is what made `HashItems.each` walk
  its cursor instead of building the pair itself.
