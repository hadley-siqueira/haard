# 0012 — Haard has function overloading, and every table is per module

Status: **partly decided**, 2026-09-01. Two things are settled and their
consequences are not. Written down early, as record 0010 was, because the
settled halves are load-bearing enough that later work should not be able to
contradict them by accident — and because one of them puts a question to two
records that are already decided.

| | |
|---|---|
| Haard has **function overloading**, with or without generic parameters | **decided** |
| The symbol table, the type table and the name table are **per module** | **decided** |
| What makes two overloads *different* | **decided** |
| That an overload set crosses the import boundary | **decided** |
| How 0008 and 0009 read for two *identical* signatures | **decided** |
| The shape of the symbol table, and where a method lives | **decided** |
| The **logic** of implicit conversion, which resolution has to rank by | **open**, and it is now its own subject |
| The `name` field of a symbol (2.5) and the type table (2.6) | **open** |

## Context

Agenda 2.5 asked how the symbol table is represented. Answering it turned up a
question that eleven records had never asked: **can two functions in one module
share a name?** Record 0009 settled what happens when two *imports* bring the
same name and said nothing about two `def`s in one file.

Hadley, 2026-09-01: *"haard tem sim sobrecarga de funções (com ou sem
parâmetros genéricos). As tabelas serão por módulo."*

## Decided: overloading exists

`def foo(x: i32)` and `def foo(x: f32)` coexist in one module, and a generic
`foo<T>` coexists with a concrete one.

**The consequence that reshapes everything below it: a name maps to a list of
symbols, not to one.** Every lookup gives back a candidate set, and the set is
narrowed by the arguments written at the call.

So **name resolution cannot finish before types exist.** It becomes two stages
— gather the candidates by name, then choose among them by signature — and
agenda 2.7 has to record both, with the second one downstream of 2.6.

## Decided: the tables are per module

The symbol table, the type table and the name table each belong to one module.

The argument is already in the ledger. Record 0004 promised that loading a
module from a blob is `resize()` and one `memcpy` per section. A table shared
across a whole compilation breaks that promise on every `.hdm` read, because
the blob's indices would have to be remapped into the current run's table
before anything could use them. Per module, a load stays a memcpy.

The price is a translation at the import boundary: a name index in A means
nothing in B. It is paid for free, because record 0008 makes an import bring
**everything** in flat, so the importer already walks the dependency's symbols
once and can build the mapping during that same walk.

This also **strengthens the sketch of agenda 3.2**, that a cross-module
reference is `(dependency index, symbol index)`. A reference by name would have
broken the moment overloading landed; a reference by index does not notice it.

## Decided: what makes two overloads different

Hadley, 2026-09-01. **Arity, the parameter types, and the generic parameters.**

**The return type does not**, as in C++ and for the same reason: a call's
arguments are known before its result is, so a return type has nothing to
resolve against at the moment resolution happens.

**A concrete declaration beats a generic one for a bare call**, and **explicit
generic arguments select the generic.** With `foo<T>(x: T)` and `foo(x: i32)`
in scope, `foo(1)` is `foo(i32)` and `foo<i32>(1)` is the generic. So the
generic is never unreachable, which was the risk when this record first said
the concrete wins in both cases.

The parser already draws that line and the ast already carries it. A call whose
callee is an `AST_GENERIC_NAME` was written with an explicit argument list; a
call whose callee is a bare `AST_IDENTIFIER` was not
(`tests/parser/cases/generic_call.hd`). Resolution reads the two as different
questions and needs nothing new from the grammar.

Note the space rule that comes with it, and it is the language's own: glued,
`make<i32>` opens a generic argument list; spaced, `make < limit` is the
comparison it looks like
(`tests/parser/cases/generic_call_spaced_is_a_comparison.hd`).

Record 0002 is where this lands in practice: generics are instantiated in the
importer, so the importer is where an overload set containing a generic
settles.

**Default parameter values make arity a range, not a number.** `parse_param`
reads a default expression and `tests/parser/cases/param_with_a_default.hd` is
the case, so `def foo(@a: i32, @b: i32 = 0)` answers to one argument and to
two. Two declarations can therefore be distinct and still both match one call,
which means **an overload set cannot be rejected when it is declared** — only a
call can be ambiguous.

## Decided: an overload set crosses the import boundary

Hadley, 2026-09-01, answering the two questions this record put to records 0008
and 0009:

- **The importer's own declaration does not hide an imported one of a different
  signature.** A's `foo(i32)` and B's `foo(f32)` are both candidates. This is
  explicitly *not* C++'s name hiding.
- **Two imports each bringing a `foo` are not a collision when the signatures
  differ.** Both are callable.

So the scope steps of record 0009 — local, class, module, imports — stop being
a sequence of *filters that stop at the first hit* for functions, and become
sources that **contribute to one candidate set**. What the ordering still
decides is the case of two *identical* signatures, below.

## Decided: the old rules survive, narrowed to identical signatures

Records 0008 and 0009 are amended and not revoked. Each keeps its rule for the
only case it can still apply to — two declarations whose signatures are the
same:

- **0008** — a module declaring `foo(i32)` and importing a `foo(i32)` uses its
  own, silently. Its rule was "the importer's own declarations win", and that
  is what it now wins.
- **0009** — two imports both bringing `foo(i32)`: the first in source order
  wins, with a warning at the use. That is the case the record was written for.

A call that matches two candidates equally well is a different thing at a
different moment — an ambiguous *call*, not an ambiguous *name* — and it is
overload resolution's to report, not the scope's.

## Decided: one flat symbol table per module, and a method points at its class

The symbol table is a single flat vector per module. A method is a symbol in
that same vector, carrying the symbol index of the class it belongs to.

Semantically the alternative was indistinguishable, and two records already
decided it:

- **0004** promised that loading a module is `resize()` and one `memcpy` per
  section. A table per class makes the number of sections depend on how many
  classes the module has.
- **3.2** sketched a cross-module reference as `(dependency index, symbol
  index)`. Per-class tables would need a third index, or a flattening into one
  numbering — which is the flat table reached the long way round.

So record 0009's "enclosing class" step of the lookup is a **filter** over the
module's table and not a search in another one, and the class symbol carries
the span of its members so the filter is cheap.

## Open, and it is now its own subject: the logic of implicit conversion

Hadley, 2026-09-01: Haard **has** implicit conversion, *"mas não vamos fazer
como C++, será outra lógica."*

That matters here because overload resolution has to rank candidates by it: with
no implicit conversion at all, resolution is exact match on the parameter type
list and ambiguity is nearly impossible; with C++'s, it is a ranking of exact
against promotion against conversion, and it is the origin of every
incomprehensible error message C++ produces. Haard's answer is neither, and it
is not designed yet. It is agenda item 1.13.

## Open: two decided records now say less than they mean

Both state their rule in terms of "a name". With overloading each has to be
restated in terms of a name **and** a signature, and neither restatement is
obvious.

**Record 0008** — *"the importer's own declarations win. A name declared in A
shadows the same name coming from any import, with no error and no
ambiguity."* Does A's `foo(i32)` hide B's `foo(f32)` completely? That is C++'s
name-hiding rule, and it is the single most surprising thing about C++ overload
sets. The alternative is that the importer's declarations join one candidate
set with the imported ones, and only an exact-signature clash is a problem.

**Record 0009** — *"between two imports, the first one in source order wins"*,
with a warning at the use. Two imports each bringing a `foo` may not be a
collision at all: with different signatures they are two overloads and both
should be callable. The warning belongs to the case where two candidates are
*equally good* for one call, which is a different rule at a different moment.

Neither record is amended here. Both carry a note pointing at this one.

> **Continued by [0013](0013-the-symbol-table.md), 2026-09-02.** Agenda 2.5 is
> now decided: a scope is a list of names, a name is a list of candidates, the
> name is an interned index carrying its hash, and an import does **not** merge
> — a lookup searches the dependency's own table. The translation this record
> called "paid for free at the import" therefore happens at every lookup
> instead, which is what the stored hash is for.

## Consequences already visible

- A symbol is not addressable by name inside the compiler. Everything that
  refers to one uses an index, which the on-disk sketch already assumed.
- Overload resolution is a phase that does not exist yet and has no agenda item.
  It belongs after 2.6 and it is not 2.7's name resolution, though 2.7 has to
  leave room for it.
