# 0038 — Everything about construction is `init`, and a `char*` reaching a `String` is a call

Status: **decided and implemented**, 2026-09-06. Two decisions, and they are
one record because the second is what made the first show.

| | |
|---|---|
| **There is no `copy` method.** The copy constructor is an `init` taking one of these | **decided**, and it **amends record 0031** |
| Nothing about construction is ever a named method other than `init` | **decided**, and it forecloses `from`, `convert` and the rest |
| `char*` → `String` stays a **named** library relation, and only the **emission** changes | **decided** (choice 1a) |
| `=` is overloadable, and joins record 0034's table | **decided** |
| `a = b` between two class values is `a.m_assign(b)`; the C++ `operator=` is gone | **decided** |

## 1. Everything about construction is `init`

Hadley, 2026-09-06:

> *No futuro, deve-se ter apenas init e destroy. O init é que deve ser
> sobrecarregado recebendo uma outra string e, de fato, funcionando como
> construtor cópia. (...) Ou seja, tudo o que for relacionado a construtor é no
> método init. Não tem construtor nomeado com copy, ou com from ou nada do
> tipo. Sempre init.*

Record [0031](0031-what-copying-a-value-means.md) gave the family a third
member called `copy`. It is retired: **the copy constructor is the `init`
whose one parameter is the class itself**, by value or by reference.

```haard
class String:
    def init : void                 # empty
    def init : void
        @text : char*               # from a char*
    def init : void
        @other : String&            # the copy constructor
    def destroy : void
```

Everything record 0031 decided **still holds** — a class that declares
`destroy` owns something, one that owns something and says nothing about being
copied may not be copied, and it is reported at the four places. Only *what
says it* changed, from a method with its own name to an overload of the one
name construction already had.

**And it forecloses a question that was open.** The note
[a-char-pointer-reaching-a-string.md](notes/a-char-pointer-reaching-a-string.md)
had a `def from` as the natural spelling for an opt-in converting constructor,
on the argument that this project prefers a **name** the language looks for.
That argument is now answered: the name is `init`, and there will not be a
second one.

**A simplified move is where this is going.** Hadley: `init` will also be
overloaded on `&&`, which does not parse yet. That is why the family must not
grow names — a move constructor called `move` beside a copy called `copy`
beside `init` is three names for one idea.

### What it cost

Three places knew the string `"copy"`, and each now looks for an `init` whose
one parameter is this class: `Coercion::declares_a_copy`,
`Emitter::copy_init_of`, and the emitter's copy-assignment test. The emitted
C++ is byte for byte the same but for the method's name — the two emitter
cases kept their exit statuses, 42 and 54.

**A class that writes only the copy `init` cannot be built with no
arguments**, and that is record [0028](0028-a-heap-array-is-a-pointer.md)
working as written rather than a new rule: `tests/emitter/cases/a_generic_names_itself`
had to grow an empty `init`, because its `copy` used to be invisible to that
question and its replacement is not.

## 2. A `char*` reaching a `String`

Hadley's rule: **only `char*` literals exist**, and when a `char*` is given to
something that expects a `String`, the appropriate **constructor** or the
appropriate **overloaded operator** is called.

| written | emitted |
|---|---|
| `let a = "abc"` | `char *a = "abc";` — unchanged, record 0022 |
| `let a : String = "abc"` | `h_String a("abc");` |
| `by_value("abc")` | `by_value(h_String("abc"));` |
| `by_reference("abc")` | `by_reference(const_cast<h_String&>(static_cast<const h_String&>(h_String("abc"))));` |
| `a = "abc"` | `a.m_operator_eq_pb12("abc");` |

**Every one of those compiled before and named no constructor.** Record 0026
emits `init` as a C++ constructor of one argument, and **C++'s own converting
constructor** ran it — which is record
[0034](0034-an-operator-is-a-method-with-an-unwritable-name.md)'s rule
surviving in the compiler's own emission, in the one place nobody looked. Haard
decided *that* it converts; C++ decided *how*.

### How wide, and why it stayed narrow

Three options were put: the pair by name (**1a**), any class with a matching
one-parameter constructor (1b), and 1b with the class opting in (1c). Hadley
chose **1a**:

> *O usuário sempre pode criar um método sobrecarregado e internamente chamar
> outro. (...) deixe o programador converter e/ou sobrecarregar manualmente
> conforme achar necessário.*

So `Coercion::is_string` stays, the compiler knows this one name, and a `Path`
that wants the same writes `abrir(Path)`, `abrir(char*)` and `abrir(String&)`
with each calling the right one. A few lines in a library, instead of a rule
that fires silently for every class in the program — which is what 1b is, and
what C++ needs `explicit` to defend against.

`char*` → `String&` is **one more entry** on record 0018's list, costing two,
and **not** the list learning to compose — record
[0035](0035-a-reference-is-the-thing-it-names.md) leaves that open still. The
temporary is a C++ one and not a hoisted local, because nothing here is
recognisable before the type phase and the sugar pass cannot hoist what it
cannot see.

## 3. `=` is overloadable, and record 0031's assignment is a method

`=` joins record 0034's table. `a = "abc"` on a `String` is the reason: without
it, C++ builds a temporary `String` and record 0031's assignment copies **that**
— two allocations and a copy, for one call.

The lookup on an assignment is **quiet**: a class that declares an `operator=`
which does not take the right side falls through to record 0031's assignment,
rather than reporting. `a = other_string` would otherwise become an error.

And record 0031's assignment stopped being a C++ `operator=`. It is
`void m_assign(T& other)` and `a = b` is a call to it, so no `operator=`
appears in the emitted C++ at all.

**Not `m_copy`**, which is what Hadley first asked for and what an assignment
looks like at a glance: record 0031 has an assignment **destroy first**, and
calling the copy alone would leak whatever the target held. `m_assign` is the
copy with the destroy that record already specified.

## Two holes it found

**A type index was read in the wrong module's table**, for the fourth time in
this project. A parameter's type belongs to the table of the module that
**declared** the function, and it was compared against one from the caller's —
which turned `__io_open_read(path)` into `__io_open_read(File(path))`. The same
class is the same **pair**, which record [0016](0016-the-type-table.md) writes
down, and never the same index.

**A value a call gave back could not be passed by reference.** `f(gives())`
where `gives` returns a class by value is an rvalue against a plain `&`, which
C++ refuses. Broken before any of this and simply never written, since nothing
had returned a class by value into a `&` parameter. The `const_cast` that fixes
it is the one record 0031's copy constructor already needs, for the same reason
— Haard has no `const` to write on a parameter (record
[0029](0029-const-is-a-convention-for-now.md)).

**And one entry was unreachable.** The *value where a reference was expected*
branch **returned** −1 when the two were not in one class chain, so nothing
below it on the list ever got a turn. `char*` → `String&` could not have been
added without noticing.

## Still open

- **Whether record 0018's list composes** (record 0035). This record did not
  need it: it added one entry rather than teaching two to chain.
- **`T&&`**, which does not parse, and the move `init` that waits on it.
- **A second library relation.** 1a's price is that the next one costs a
  second name in the compiler beside `String`.
