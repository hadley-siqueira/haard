# 0077 — An intermediate representation, for the processor and beyond

Status: **decided in part**, 2026-10-08. Nothing is built. The rest of the
design waits on the numbered questions in
[notes/ir-questions.md](notes/ir-questions.md); the whole discussion, every
proposal and why it was replaced, is
[notes/designing-the-ir.md](notes/designing-the-ir.md).

| | |
|---|---|
| An IR of our own, with no dependency (LLVM, MLIR and QBE are models, not libraries) | **decided**, Hadley |
| The C++ back end **stays** beside it | **decided**, Hadley (record 0025's portability) |
| Source variables in **memory form** first; SSA (`mem2reg`) later, as a pass | **decided**, Hadley |
| An **IR interpreter** is the test oracle: the same exit status as the C++ path | **decided**, Hadley |
| The IR's text is a **dump for reading** and goldens; nothing parses it back | **decided**, Hadley |
| Targets: his **PRET** processor (64-bit, big endian, floating point, scratchpads, dedicated registers, ISA in design), plus native **x86-64 and ARM** | **stated**, Hadley |
| Goals the IR must serve: **WCET** and real-time analyses, **scratchpad placement and overlays**, **SIMD**, a **CGRA** | **stated**, Hadley |
| Aim for the best design, even over his earlier choices; the IR **as high level as possible** | **instruction**, Hadley |
| Upper bits of a 32-bit result undefined | decided by Hadley for an untyped IR, **moot** now: a typed IR has no such bits, and each back end answers it for its ISA |
| One IR with a structured and a CFG form, typed, fields by name, objects by symbol, placement as a table, plain data by index | **proposed**, questions 78–91 |

## Context

Record 0025 chose no IR for the C++ back end and named the signals that
would make that wrong. Two have arrived: targets with real distance (the
processor, x86-64, ARM) and the work C++ did in silence (destructors and
temporaries at computed points, copies, vtables, layout of every aggregate).
On top of them came goals no transpiler serves: a WCET for a precision-timed
machine, deciding what lives in which scratchpad, and keeping loops visible
enough to vectorise them and map them onto a CGRA.

## What was decided, and what was replaced

Four shapes were proposed in two days (the notes keep each one and its
reasons): LLVM's typed CFG; QBE's level, with untyped 64-bit temporaries and
the width in the opcode; two IRs, a structured one above a QBE-level one; and
the current proposal, **one typed IR with two control forms**. Each step was
driven by a new requirement:

- **Several native targets** did not move it: a low IR serves amd64, arm64
  and riscv64 (QBE does).
- **SIMD and the CGRA** did: they need loops, induction variables, strides and
  aliasing facts, which a CFG of byte offsets has to recover by heavy
  analysis. MLIR exists for this reason.
- **Hadley's instruction to stay high level** removed the untyped level: once
  tables of structs, classes and layout exist, types cost little and give the
  verifier its rules.
- **WCET** made the structured form central (a loop region carries its
  bound), made whole-program compilation necessary and added flow facts,
  timing barriers and a machine ↔ IR ↔ source mapping.
- **Scratchpads** made every object in memory a named entity, referred to by
  symbol, with placement as a table applied at the end, so placing and
  analysing can iterate.

## Consequences

- The next step is not code: it is the answers to
  [notes/ir-questions.md](notes/ir-questions.md). The language questions
  (blocks A and E) become records of their own; the rest amend this one.
- Hardware questions left "open" touch only the back end; the IR can start
  without them.
- When the answers are in, this record is rewritten as **decided**, and the
  order of the work in the notes (section 11) starts.
