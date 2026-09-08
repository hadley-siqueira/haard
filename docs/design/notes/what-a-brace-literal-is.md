# What `{name: 'Hadley', age: 23}` is

Written 2026-09-03, when Hadley asked what a hash key is and the answer turned
out to be a different question. **Nothing here is decided.** Agenda 1.23.

## What forced the question

`hdc b.hd` reported `cannot find 'name' in this scope` for the key of a brace
literal. The key was going through the UseResolver's scope chain like any other
identifier, which is wrong under every reading below — so **that much is
already fixed**, and it is the only part that is: `AST_HASH_PAIR` walks its
value and not its key.

## The fact that split it in two

```
{ name: 'Hadley', age: 23 }
```

The values are a `char*` and an `i32`. **A `Hash<K, V>` has one `V`**, so in a
statically typed language with no dynamic value — and record 0018 kept one out
of the whole language — this literal is not a hash. It is a record with named
fields, and the hash is a different construct that happens to share the
brackets.

## Hadley's design, 2026-09-03

**The brace literal is a composite literal read by its context**, which is
record 0018's first rule generalised from a scalar to a composite:

- **With a type written**, `let p : Person = {name: 'Hadley', age: 23}`, it
  initialises that type's fields. The keys are **field names** and are checked
  against the declaration: a key naming no field is an error, and each value is
  checked against its field's type. Nothing new is needed in the type system —
  it is the `expected` the typer already carries into every expression.
- **With none**, `let p = {name: 'Hadley', age: 23}` builds an **anonymous
  struct**: a real, static type, inferred structurally from the literal.
  `p.age` is an `i32` and `p.name` a `char*`, both known at compile time. The
  field values may be any expression — `{name: user_name, age: compute()}` is
  ordinary code — because it is the **type** that is static and not the values.

**And the dynamic JSON object is a library type, not a language one.**
`json({name: 'Hadley', age: 23})` — a generic function monomorphised on the
anonymous struct — is what builds a real dynamic JSON value, with the tagging
and the runtime inspection that implies. The language never grows a dynamic
value; a library does, out of ordinary Haard, and the boundary is **written at
the call site**.

## Why that division is the right shape

It is the move record 0023 already made for `char*` → `String`: **the language
stays small and the library does the work.** It keeps record 0018's rule 4
intact — no dynamic value, no variant, no runtime type in the language — and it
puts the cost where a reader can see it. `json(...)` is greppable; an implicit
conversion to a tagged value would not be.

It is also what the two languages that solved this well do. Zig's
`std.json.stringify` and Rust's serde both take a static type and produce the
dynamic form through a library, and neither language has a dynamic value of its
own. The languages that took the other road — a JSON object as a language
primitive — are all dynamically typed, where there was no static struct
competing for the syntax.

## What it costs, and this is the part to weigh

**1. `json(T)` has to read T's fields at compile time — names and types — and
Haard has no compile-time reflection.** Zig has `@typeInfo`, Rust has derive
macros over the syntax tree. This is the real price of the design. It is a much
better-behaved feature than a dynamic value (it runs at compile time, it costs
nothing at run time, and it cannot make a call mean something a reader cannot
see), but it is a feature and it is not small. **It is the thing to decide
first when this subject reopens**, because without it the dynamic half of the
design has no way to be written.

**2. An anonymous struct has no declaration, and record 0016's `Type` assumes
one.** A `TYPE_NAMED` holds `(module, candidate)` — a pointer at a declaration
in a symbol table. An anonymous struct has no `class` anywhere to point at, so
it needs either a synthesized declaration or a new type kind carrying its
`(name, type)` pairs in the argument pool. The pool already interns
structurally, which is most of the work: two literals with the same fields in
the same order would be one type for free.

**3. Field order, and whether it is part of the type.** Is `{name: 'x', age: 1}`
the same type as `{age: 1, name: 'x'}`? Order-sensitive is what the type table
gives without being asked and what a struct layout wants; order-insensitive is
what a reader of a JSON-shaped literal expects. Unanswered.

**4. Ownership reaches in here too.** `{name: 'Hadley'}` holds a `char*` into
the `.rodata` today. If a field is declared `String`, record 0023's coercion
runs a constructor, and then the anonymous struct owns something and agenda
1.18's `init`/`destroy` is what says when it is freed.

**5. `{}` would read three ways**: a fixed array (`{1, 2, 3}`, record 0021), a
record, and a hash. The first is told apart with no lookahead at all — it has
no `:` — so the discrimination stays syntactic and local, which is the test
that matters. Whether the record and the hash can share it is the open half.

## Why it is deferred

**The half that is hard is a question about runtime representation, and there
is no emitter to answer it against.** Choosing what a dynamic JSON value costs
— its tag, its layout, its allocation — before anything at all can be run is
choosing blind. It is the same shape agenda 1.18 has, and record 0023 deferred
that one for the same words.

Deferring costs nothing else: the brace literal **types to nothing** today
(there is no `AST_HASH` case in the typer at all), nothing in the standard
library depends on it, and no test asserts a meaning for it. Everything above
is additive to what compiles now.
