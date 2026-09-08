# 0032 — How a template string is lowered

Status: **decided and implemented**, 2026-09-06. Two questions were open
inside record 0025's Ast → Ast pass; both are answered here. The second
answer was *wait for the prelude*, and record
[0033](0033-the-prelude-is-a-list-of-automatic-imports.md) delivered it the
same day, so the wait was hours. `src/haard/sugar/sugar_lowerer.{h,cpp}`,
`tests/sugar/` and `tests/programs/cases/shapes_and_text_by_template`.

| | |
|---|---|
| A template string becomes a **local `String`** and a run of `append` calls, hoisted before the statement it was written in | **decided** |
| Hoisting where it would change the meaning — an operand of `and`/`or`, a loop condition — is **refused**, and loosened later | **decided** |
| How the pass names `String` in a module that never imported it | **decided: the prelude answers it**, record 0017. Template strings wait for it |
| A user's class in a `${}` is found by a `to_string` convention | already decided, record [0031](0031-what-copying-a-value-means.md) |

## Where this stood

The front end has been complete since 2026-09-05. The scanner emits
`BEGIN CHUNK (INTERP CHUNK)* END`, the parser builds `AST_TEMPLATE_STRING` with
the real expressions under it, and the PrettyPrinter reprints it exactly.

Nothing after that knows the node exists. `AST_TEMPLATE_STRING` appears in the
scanner, the parser, the builder and the printer, and **nowhere else** — not in
the ExpressionTyper, not in the StatementChecker, not in the emitter. So

```haard
let s = "value is ${n}!"
```

passes `hdc file.hd` in silence and dies in `--emit-cpp` with *'s' has no type
the emitter can write*. It is the tenth instance of this project's one recurring
shape: a construct the front end accepts that no later phase was ever taught,
found by writing ordinary Haard and running it.

The blocker was never the `String`. Record 0031 answered that from the other
end, and `String` is written, owns its bytes and is **its own builder** —
`append` is overloaded for `char`, `char*`, `String&`, `i64`, `i32`, `u32`,
`bool` and `f64`, so nothing else has to exist.

## Decision 1 — a local, hoisted, and refused where it is not safe

The pass declares a local `String` before the statement the template string was
written in, appends each chunk and each `${}` in order, and replaces the node
with a use of that local.

```haard
out->writeln("lit pixels: ${lit_count(shapes)}")
```

becomes the tree for

```haard
let __ts0 : String
__ts0.append("lit pixels: ")
__ts0.append(lit_count(shapes))
out->writeln(__ts0)
```

**Why a local and not a chain.** The other shape is to make `append` give back
a `String&` and lower to one expression, `String().append(a).append(b)`, which
would need no hoisting at all and get the short circuit right for free. It was
rejected for two reasons, and the second is the real one: it changes the
library to suit the sugar, and it needs an Ast shape that does not exist —
an **unnamed stack temporary**. `new String(...)` gives a pointer, and every
`String` a Haard program has today is a local with a name. Hoisting uses only
shapes that already work.

**Why refusing is right.** Hoisting is not always meaning-preserving:

```haard
if p != null and check("name is ${p->name}"):     # runs the append anyway
while more("still ${n} left"):                    # built once, not per pass
```

Lifting the appends out of the second operand of `and` runs them when the
first was false, and out of a `while` condition runs them once. So the pass
**reports** a template string in either position rather than emitting something
that means a different program, in the same words the rest of the compiler
uses. Both can be loosened later — an `and` by lowering to the branch it
already is, a loop condition by rebuilding at the top of the body — and
loosening is additive, the way record 0018's list is.

## Decision 2 — the name `String` waits for the prelude

The pass has to write a type the source never wrote. In a module that never
imported `std.string`, `String` names nothing, and there were two ways out:

- **require the import**, and report *a template string needs `String` in
  view* where it is missing;
- **let the prelude declare it**, record 0017, where a name in scope in every
  file without being imported is already the decided mechanism.

Hadley, 2026-09-06: **the prelude.** So this is not a *how*, it is an
*ordering*: template strings are the first construct whose desugaring needs a
name of its own, which is exactly the case record 0017 exists for. `T[]` →
`Array<T>` is the same problem and was the reason that record was written.

Requiring the import would work today, and it was rejected on the same ground
record 0008 is: sugar that only compiles when the user has written an import it
never mentions is a name nobody can grep for, with a diagnostic teaching a rule
that is about to stop being true.

**Consequence: the prelude moves ahead of template strings** in the order of
work. It was item 4 and is now what unblocks item 2.

**Amended the same day by [0033](0033-the-prelude-is-a-list-of-automatic-imports.md),
which built it.** This record said the prelude would have to **declare**
`String`, answering record 0017's open point 4. That is no longer the shape:
there is no prelude module, so it declares nothing. The table carries a list of
imports every module of the program is given, `String` stays an ordinary module
of the standard library, and the lowering pass finds it the way any name is
found. What this record needed — a `String` the pass can name in a module that
never wrote an import — is delivered, and by less.

## What this does not change

Nothing is added to the language and nothing is added to `String`. The pass is
record 0025's Ast → Ast, run before the type phase, and every phase after it
reads an ordinary tree: a local, some method calls, an argument. Overload
resolution picks the `append` for each `${}` by its type, which is what makes
the eight overloads carry the whole feature; record 0031's `copy` is what makes
handing the local to something work; and `to_string` is what a user's class in
a `${}` reaches, also already decided.

## What building it added

### A token whose text is not in the file

`Module::get_token_value` slices the **source file's content** by an offset and
a length, so a name the source never wrote has nowhere to live — and the pass
writes three of them: the local, `String`, and `append`.

They are kept beside the token stream instead, in a vector, with the first
synthetic index recorded so the lookup is a subtraction and not a map on a path
every phase walks. The token's **offset is still used**, for the one thing it
is still good for: **where to point**. Each synthetic token is given the
position of a real one from the construct being lowered — the chunk, or the
interpolation — so a diagnostic about a node nobody wrote still underlines
something the author did write. *"no 'append' takes these arguments"* about a
class with no `to_string` lands on the `${}` that holds it.

`Module::add_synthetic_token(kind, text, like)`, and every one of them is made
inside this pass, before any phase reads token text.

### The node is rewritten in place

A template string **becomes** the identifier that uses the local: same node
index, same kind field overwritten, children cleared. So no walk has to know
its parent, and its sibling link and whatever points at it are untouched. The
alternative — finding the parent and splicing — is work in exchange for
nothing.

### A refused template string is still a String

This was not planned and the refusals are worse without it. An expression that
types to nothing makes the call around it report *no `f` takes these
arguments*, so a refusal read as **two** errors: the refusal, and a consequence
of the refusal.

So a refused one is still lowered into `let __tsN : String`, with the appends
**dropped**, and the node still becomes a use of it. That is recovery — the
parser's poisoned primitive one phase later — and it is safe for the same
reason: an error was logged, so nothing is emitted from this tree. Dropping the
appends is what keeps the recovery from inventing errors of its own:
`for i = 0; f("${i}"); ...` would otherwise put a use of `i` before the loop
that declares it.

Module level is the one refusal with no recovery, because there is no block to
put the declaration in.

### The pass runs before the symbols are collected

It writes a **declaration**, so it has to. `Compilation::build`'s module loop is
`resolve_imports`, then `lower_sugar`, then `collect_symbols`. Everything after
that point reads an ordinary tree, and nothing else in the compiler learned
that a template string exists: overload resolution picks the `append` for each
`${}` by its type, which is what makes String's eight overloads carry the whole
feature.

### `char*` to `String&` is two coercions, and nothing has decided they chain

Found writing `shapes_and_text_by_template`. A message with no `${}` in it is a
`char*` (record 0022), and a `String&` parameter is one coercion too far —
`char*` → `String` (record 0023) and then value → reference (record 0031's
amendment) are two entries of record 0018's list, which is a list of **single**
steps.

Taking the parameter **by value** works and is what the case does. Whether the
list should compose is a question for record 0018 and is not asked here.

## The case that was blocked on it, and now is not

`tests/programs/cases/shapes_and_text_by_template` is
`shapes_and_text_are_drawn` again with the sugar, and **the drawing is
identical on purpose**: the picture in the two goldens is byte for byte the
same, so a diff between the cases is exactly what template strings changed.

What they changed is that a message carries the numbers it is about. Three
statements and a helper —

```haard
out->write("lit pixels in the shapes preview: ")
write_u32(out, lit_count(shapes) as u32)
out->write('\n')
```

— became one line. `File` grew `write` and `writeln` for a `String&` so that a
`${}` at a call site does not have to end in `.text()`.

## What is left open

**Loosening the three refusals**, which record 0018's own argument says is
additive: an `and` can be lowered to the branch it already is, and a loop
condition by rebuilding at the top of the body. Nothing needs deciding first.

**The name `__tsN` is writable by a user**, unlike record 0002's `Pair#3#7`,
which no source can spell. A program declaring its own `__ts0` in the same
scope would collide. It follows record 0030's `__io_` convention and the
leading underscores are the whole of the protection.
