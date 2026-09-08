# 0011 — `Compilation` holds the modules, `Module` is a file, `Context` is gone

Status: **decided**, 2026-09-01. The name `Compilation` is settled *for now*, at
Hadley's word: "podemos prosseguir com o nome Compilation por enquanto.
Qualquer coisa mudamos depois se acharmos um nome melhor." What the class *is*
does not depend on what it is called, and everything below stands under a
rename.

## Context

Agenda 2.1 asked for three names at once: what the container of many modules is
called, what one module is called, and what replaces `Context` — a class that
was doing all three jobs at different times. The item stayed open while there
was only ever one module in memory, which made the question theoretical. The
module loop made it concrete: the code needed a container, and the container
needed a name before it could be written.

## Decided: `Module` is a file

Unchanged from record 0001 and already in the code — `Context` was renamed
`Module` on 2026-08-25. A module is one `.hd` file: its source, its tokens, its
ast and its logger, plus the two identities the loop gave it, its dotted name
and the root it sits in.

The consequence worth stating: **a module's name is its path**, so nothing else
has to carry it, and two modules are the same module exactly when their
resolved paths are the same.

## Decided: `Compilation` is every module of one program, and the loop

`src/haard/compilation/compilation.{h,cpp}`. It owns the `ModuleFinder`, the
modules, and the loop that goes from the entry file to everything its imports
reach. Nothing else in the codebase holds more than one module.

The `Driver` keeps the command line, the phase order and the exit code, and now
holds a `Compilation` where it used to hold a single `Module`, a `Scanner` and a
`Parser`. That is the line: **the `Driver` owns the process, the `Compilation`
owns the program.**

## Decided: `Context` does not come back

The name was doing three jobs and named none of them. It is gone from the code
and it should not return for the symbol table, the type table or the scope
stack — each of those is its own thing with its own name (agenda 2.5, 2.6, 2.7).

## Why the name is provisional, and what would change it

`Compilation` is the word the agenda itself used throughout, which is why it
was reached for. Two things could still unseat it:

- **If a compilation ever holds more than one program** — a library and its
  tests, say, built together — then the class is a *workspace* and the word
  `Compilation` is what one of its members is.
- **If the on-disk work (Track 3) gives the word another meaning**, for instance
  if "a compilation" comes to mean one run of `hdc` rather than the set of
  modules it worked on.

Neither is on the table. Renaming later costs a search and replace, and this
record is written so that a rename is a rename and not a re-decision.

## Consequences

- The class exists before agenda 2.3 (the module pool) is decided, so it owns
  its modules with plain `new` and `delete` and keeps them for its lifetime.
  Reuse, recycling and eviction are 2.3's to answer, and nothing here blocks
  them: the modules are behind `get_module(index)` and nothing outside holds a
  pointer across a build.
- The interning map is keyed by the **canonical resolved path**, which is the
  identity agenda 3.4 will hash. A path is what gets opened; a key is what says
  two files are one module.
