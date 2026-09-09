# 0054 — A generic function is instantiated per call

Status: **decided and mostly built**, 2026-09-09. Hadley's, and it came out of
asking why `Array<T>` had an `operator==` at all.

| | |
|---|---|
| `Array<T>` and `List<T>` **lose** `equals`, `==` and `!=` | **decided**, Hadley |
| Comparing two containers belongs in a **free generic function** | **decided**, Hadley |
| `f<i32>(3)` instantiates the function, by the machinery that already existed | **built** |
| The phases **come round again** for a clone born while they run | **built** |
| One shape is still open: a `let`-inferred call whose body calls a method on `Bag<T>` | **open** |

## The question that started it

*"Por que Array tem o operador `==`? Qual caso de uso está necessitando disso?"*

Nothing. Measured across the whole repository: **no program has ever compared
two `Array`s or two `List`s.** The only calls to `equals` were the
`operator==` and `operator!=` sitting beside it in the same class. It was
written because record 0034 made operators overloadable and comparing a
container looked natural.

And it was the single reason `Array<Token>` did not compile. A method of a
generic class is instantiated **with the class**, so an `equals` that compares
two `T` made every `T` ever put in an Array need an `operator==` — for a
method nobody called:

```
error: Token has no 'operator=='
  --> std/array.hd:85:29
```

A file the reader never opened, about a method they never called. That is what
`Array<Token>`, `Array<AstNode>` and `Array<Type>` — the bootstrap — each ran
into.

## The decision

**Take it out of the class.** A **free** generic function is instantiated per
call, so it asks nothing of a `T` nobody compares:

```haard
def same<T> : bool
    @a : Bag<T>&
    @b : Bag<T>&
    ...
```

This is Rust's answer without Rust's machinery. Rust writes
`impl<T: PartialEq> PartialEq for Vec<T>` — the method exists only when the
bound holds. Haard has no bounds and no interfaces, and does not need them for
this: putting the method where it is instantiated **on use** buys the same
thing.

Both classes now carry a comment saying the absence is the decision, so the
next person to notice `Array` cannot compare finds an answer.

## What had to be built, and it was one missing caller

A generic function had **always parsed** and had **never been instantiated**.
`ExpressionTyper::callee_of` opened with

```cpp
if (kind_of(node) == AST_GENERIC_NAME) {
    node = first_child(node);
}
```

— the type arguments dropped on the floor. So `f<i32>(3)` was ranked against
the **unbound** signature, `T -> T`, and an `i32` does not match a `T`. The
answer was *no 'f' takes these arguments*, and `zero<i32>()` "worked" only
because a signature with no `T` in it has nothing to fail to match.

The Instantiator could always have done it: it clones a declaration node and
asks nothing about its kind, and `SymbolCollector::collect_declaration` takes
`AST_FUNCTION` as its first case. `TypeBuilder::instantiate_written` is the
entry that was missing, and `ExpressionTyper::instantiated` is the caller:
every candidate that is a generic **function** is replaced by its clone, and a
candidate with no type parameters is left alone so an ordinary overload of the
same name still competes.

**And it is not an instantiation when an argument is still a parameter.**
`same<T>(a, b)` written inside another generic would clone `same` with `T`
unbound — a declaration whose body is then checked with nothing bound and
reported about. The same guard a written type has.

## The extra round of phases

The pipeline was `collect_types` → `resolve_uses` → `check_statements`, each
running once per module. A call carrying type arguments **instantiates**, and
a call is checked in the last phase as often as it is typed in the first —
`return f<i32>(3)` reaches only the checker. So a clone can be born after its
module's turn has passed, and nothing would ever type its body:

```
hdc: 'a' names no declaration
```

about a parameter written two lines above it.

So `check_statements` is a loop now, and it is the module loop's shape a
fourth time. The type phase knows what is new by its own mark and does nothing
when nothing is; the checker keeps a **set of the declarations it has walked**,
so a second round costs nothing and no diagnostic comes out twice.

**The snapshot is the part that matters.** A clone is *appended* to the root
as it is made, so walking the declaration list live reaches it in the same
round — before the type phase has been through its body, which is the one
thing the round exists to arrange. The list is read into a vector first, and
whatever arrives after waits for the next round.

## What was found on the way

**`UseResolver::use()` writes no resolution.** It resolves a name only to
check that it exists, and reports when it does not; the ResolutionTable is
written by the **type phase**. An afternoon went into resolving a fresh
clone's uses before that was measured, and the work was reverted rather than
left in looking useful. A clone needs its body **typed**, not resolved.

## What is still open

One shape does not work:

```haard
let bound = same<i32>(xs, ys)     # inferred by the type phase
```

when `same<T>`'s body calls a **method** on `Bag<T>` — `a.at(i)`. Written as
`if same<i32>(xs, ys)` it works; with `a.held[i]` instead of `a.at(i)` it
works; with two different instantiations it works; with a generic calling a
generic it works; with `Array<T>&` and `a.length()` it works. It is the
combination, and it is not understood yet.

What is measured and passing is in
`tests/emitter/cases/a_generic_function_is_instantiated_per_call`, whose
verdict is the exit status of the binary it built.

The likely shape of the answer, for whoever picks it up: the clone is made
during **inference**, and what is not typed by the time its body is checked is
a method of the *other* clone it names. Every bug of this family today has had
the same cause — a clone made after a phase never gets that phase — and this
record added the round that fixes it for the statement checker. This one is
one layer in from there.
