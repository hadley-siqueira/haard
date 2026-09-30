# 0066 — A move is written

Status: **decided and built**, 2026-09-30. Hadley: *"T&& é move com o
construtor que você mencionou ao lado do de cópia. O compilador escolhe move
quando o usuário escrever explicitamente &&expressao. Da mesma forma que
pode-se escrever &var, deverá ser possivel escrever também &&var, que seria o
equivalente a um std::move(var) de C++"*. Three questions that followed were
answered the same day. Makes real what record
[0016](0016-the-type-table.md) reserved (`T&&` is an rvalue reference, a kind
of its own) and closes what record [0031](0031-what-copying-a-value-means.md)
left open (*"Moving. There is none"*).

| | |
|---|---|
| `T&&` is C++'s rvalue reference; a class says how it is moved with `init(other : T&&)`, beside the copy `init(other : T&)` | **decided**, Hadley |
| The compiler moves **only where `&&x` is written** -- `&&x` is `std::move(x)` | **decided**, Hadley |
| `&&x` on a class with no move `init` is the **copy**, as in C++ | **decided**, Hadley |
| `y = &&x` **moves**: destroy y, then the move `init` -- record 0031's assignment with the move where the copy was | **decided**, Hadley |
| A **temporary is never a copy**: a value built from one is built in place, for every class | **decided**, Hadley |
| An assignment from a temporary is still a copy, since the target already exists | **decided while writing it** |
| A name of a `T&&` reads as a `T&` | **decided while writing it**, C++'s rule |

## The syntax

```haard
class Buffer:
    data : u8*

    def init : void
        @other : Buffer&&

        data = other.data
        other.data = null

let b = &&a          # moved
take(&&b)            # moved into a parameter taken by value
c = &&b              # c destroyed, then moved into
return &&made        # moved out
```

`&&` is one token everywhere. As a type postfix it is `T&&` and **never**
`(T&)&` -- the trap record 0016 named, since `**` *is* read as two pointers.
In front of an operand it is a move; between two operands it is still the
logical `and`, which lives at a level far above. The printer writes `& &x` with
its space, since glued it would come back as a move -- and needs nothing new
for it: `would_paste` already keeps two tokens from gluing into a third.

## What the compiler does, and what it never does

It moves **where `&&` is written and nowhere else**. The move `init` is
therefore not a C++ move constructor: g++ picks one of those on its own --
returning a local where NRVO cannot apply, building from a temporary -- and
that would be the compiler deciding to move. It is emitted as a constructor
with a **tag first**:

```cpp
struct __haard_move {};

Buffer(__haard_move, Buffer&& other);
```

No C++ rule ever supplies an `__haard_move`, so the constructor runs only
where this compiler writes `Buffer(__haard_move(), ...)`: a binding, an
argument, a `return`, a conversion, whenever the value given is a `T&&` and
the class has a move `init`. `&&x` itself is `static_cast<T&&>(x)`, which is
what `std::move` is and needs no `<utility>`. The tag is declared only by a
program that writes a move `init`, so every other golden is byte for byte what
it was.

`tests/emitter/cases/a_move_happens_where_it_is_written` returns one of two
locals, where NRVO cannot apply and C++ would move -- and it copies.

## In the type phase

- `&&x` needs a **place**, for the reason `&x` does: `&&make()` names nothing
  whose insides could be taken. Its type is `T&&` of what `x` is, through a
  reference.
- A `T&&` reaching a `T` is built by the class's move `init`. That is record
  [0046](0046-a-conversion-is-a-constructor-the-compiler-calls.md)'s entry
  unchanged -- *a value converts by the class's own `init`* -- since the move
  `init` takes exactly a `T&&`. With no move `init` it is the copy, one step
  further.
- Anywhere else a `T&&` is what it names, a `T&`, **one step further**, so an
  overload taking the `T&&` always wins over one taking the `T&`.
- **Nothing goes the other way**: a `T&&` parameter is given a `&&x` and
  nothing else. `wants_a_move(a)` matches no overload.
- A **name** of a `T&&` -- the move `init`'s `@other` -- reads as a `T&`. It is
  a place with a name, and moving from it again is `&&other`, written.

Record 0031's refusals -- a class that owns something and writes no copy
cannot be copied -- are asked at the same four places as before and let two
things through: a `&&x` into a class with a move `init`, which is not a copy,
and a **temporary**, which is built where it goes. So a class that can only be
moved is usable: `let m = make()`, `take(make())`, `return &&made`. What it
refuses is still every copy: `let f = a`, and `a = make()` -- an assignment
has a target that already exists, and filling it from a temporary would be the
compiler choosing to move.

## Found on the way

`value_of` reads through a `T&&` as it does through a `T&`, which is right --
and which made the emitter's `copy_init_of` take the move `init` for the copy,
since it compared the parameter's `value_of` to the class. It skips a `T&&`
now.

## In code

`AST_MOVE`, `AST_MOVE_REFERENCE_TYPE` and `TYPE_MOVE_REFERENCE`;
`ExpressionTyper::move` and `is_temporary`; `Coercion::is_moved` and the
`T&&` entry in `Coercion::steps`; in the emitter `is_move_init`,
`move_init_of`, `emit_moved` and `emit_move_assignment`. The bootstrap's
parser and printer read and write both forms.

Cases: `tests/parser/cases/a_move_is_written_with_two_ampersands.hd`,
`tests/type_table/cases/a_move_is_written_and_never_inferred` and
`tests/emitter/cases/a_move_happens_where_it_is_written` (42, under the
address sanitizer too). The line `let b = gives_back()` left
`tests/type_table/cases/a_value_that_owns_is_not_copied`.

Ten sabotages, each caught: a `T&&` name left a `T&&`, no tag constructor,
the tag never written at a move, no move assignment, a temporary refused at a
call of a value and at a binding, a returned move checked as a copy, the move `init` taken for the copy, no fallback to the copy, and a move
of what is no place. One more was not caught and taught something: removing a
spacing rule from the printer changed nothing, because the rule was never
needed -- so it was deleted.
