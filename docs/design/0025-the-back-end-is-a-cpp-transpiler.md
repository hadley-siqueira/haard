# 0025 — The back end is a C++ transpiler, emitted from the tree

Status: **decided**, 2026-09-03, and the first version is implemented.

| | |
|---|---|
| The back end emits **C++**, and a C++ compiler makes the binary | **decided** |
| It emits from the **Ast and the ResolutionTable**, with no intermediate representation | **decided**, *for this back end* |
| Every name is **mangled**; a method by its name and parameters, everything else by where it was declared | **decided** |
| Sugar is taken apart **here** until something needs a name the source never wrote | **decided** |

## Context

The front end checked and emitted nothing: the whole result was an exit code
and diagnostics. Nothing decided in twenty-four records had ever run.

Hadley, 2026-09-03, on why C++ and not a real back end yet: it is not only that
the pipeline is unfinished. **Any architecture with a C++ compiler becomes a
target** — haard → C++ → cc → binary — and that portability is worth having on
its own. Readable output is explicitly not a goal.

**He also settled what comes after:** an intermediate representation *will* be
needed, for a processor he is building. So this record decides the shape of
*this* back end and says what would make it the wrong shape, rather than
claiming an IR is never wanted.

## Decision

**1. No intermediate representation, for this back end.** An IR exists for two
reasons and neither applies here.

*To cross distance.* C++ has expressions, control flow, classes, inheritance,
overloading and destructors, so the constructions correspond one to one.
Lowering exists to reach something lower than the source, and this is not
lower.

*To do dataflow.* SSA, a control flow graph, liveness, definite assignment.
Nothing here needs any of it: the C++ compiler optimises, and every check this
language makes is over the tree and already done.

What normally makes an AST an unusable source for emission is that it does not
know the types or which declaration a name meant — so the emitter would
redo the work. **Record 0019 already solved that**, and it said so at the time:
the ResolutionTable carries a type and a declaration per node, and its own
comment names the emitter as the reader that needs the second. Emitting from
anything else would throw away the table that exists for this.

Record 0002 also keeps generic instantiation in the importer, so the emitter
never sees a `T`.

**2. Every name is mangled.** `h<module>_<candidate>_<source name>`.

Not for tidiness. **Record 0010 puts two versions of one library in one
program**, so two modules declare `zip.compress` and they are different code —
source names collide and the linker picks a body. And **C++ resolves overloads
by its own rules**, which are not record 0018's: no numeric conversion here, a
ranking by inheritance depth, a literal's distance from its default. A call
this compiler resolved to one candidate could resolve to another there, in
silence. A candidate's own name means C++ never chooses anything. A C++ keyword
can never be produced either, which is why locals and parameters go through it
too.

**A method is named differently, and it has to be:** `m_<name>_<parameters>`.
Record 0020 says a derived class writing a base's name and parameters
**overrides** it, and in C++ two virtual functions with different names are two
functions — the derived body compiles, links, and is never called through a
base. Naming a method by what makes it that method *is* record 0020, written
once, and it needs no walk up the chain to find the base it overrides. The
return is out of it, which is also what lets record 0020's covariant return
work.

**3. Four sections in one file.** Haard allows use before declaration and C++
does not, so the output is ordered rather than the input: every type forward
declared, then the types, then every function prototyped, then the bodies.
Inside the second section order still matters — a base and a field held by
value must be complete — so it is a walk over those two edges, and a cycle is
reported rather than emitted.

**4. Sugar is taken apart in the emitter, while it stays local.** `elif` is
`else if`; `and`, `or` and `not` are `&&`, `||` and `!` in either spelling; a
`.` on a pointer is `->`, which is record 0018's third coercion and a question
about the left side's type and never about what was written.

When something needs a name the source never wrote, or a statement where an
expression was, it wants a **tree-to-tree pass** before this one and not a
bigger switch inside it — and not an IR either. The reason is specific to this
project: the PrettyPrinter is already an oracle for a tree and the parser suite
already compares trees, so an Ast → Ast pass is testable with what exists,
while an IR would need a dumper and a suite of its own.

## Consequences

- **The suite's verdict is the exit status of the program it built.** Every
  case emits, `g++` compiles it, the binary runs, and the golden's last line is
  what it returned. The generated text is in the golden so a change is visible,
  but a transpiler is right when its output runs and gives the right answer.
  `tests/emitter/`.
- **Everything it cannot do says so.** A kind the emitter does not claim
  reaches the reader as a sentence, never as C++ that means something else. A
  refusal is a golden like any other.
- **What it does not claim yet**: a hash literal, a tuple, a list, a closure, a
  template string, a range, a symbol, `T[]` with no length, and `for x in a..b`.
  Most of those wait on a decision or on the standard library rather than on
  work.
- **`for x in a..b` is deliberately absent** rather than unwritten. A range has
  no type, so the loop variable has none, so **no program using one gets past
  the type phase** — an emitter for it would be code no test could reach.
- **`init` and `destroy` are still nothing** (agenda 1.18), so a class is a
  plain aggregate. This is where that stops being an emitter-shaped question
  and becomes an emitter-blocking one.

## What would make this the wrong shape

Named so they are recognised when they arrive, since Hadley has already said
the first one will:

1. **A second target** — the processor he is building, or LLVM. Real distance,
   real lowering. The Ast → Ast pass is the migration path, not a dead end.
2. **A check that needs flow** — definite assignment, "not every path returns",
   lifetimes. That wants a control flow graph and a tree is not one.
3. **Memory management with frees inserted at computed points.** If
   `init`/`destroy` become C++ RAII, C++ does it; anything else moves.

## In code

`src/haard/emitter/emitter.{h,cpp}`, and `hdc --emit-cpp`. Four cases in
`tests/emitter/`, and the one that carries the most is
`classes_inheritance_and_overriding`: 145, reached through a base pointer,
through a bare name inside an inherited method, and through a `.` on a pointer,
with an overloaded free function on the side. Naming a method by its candidate,
dropping `virtual`, dropping the module from a free name, or writing `.` where
`->` was needed each move that number.
