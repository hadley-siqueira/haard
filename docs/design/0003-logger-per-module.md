# 0003 — One logger per module, merged in module order

Status: decided, 2026-08-24

## Context

The logger sits in `Context` today and holds a `SourceFile*` so it can turn an
offset into a line and a column. With many modules, and one day with many
threads, it could equally have been one global logger behind a mutex.

## Decision

Every module owns its logger. The diagnostics are merged when the compilation
ends, **in module order — not in the order they were produced**.

## Consequences

- A global logger behind a mutex was rejected, but *not* for the reason it
  looks like. Diagnostics are a cold path: an error costs far more in
  formatting than in lock contention, so the mutex would not have hurt.
- It was rejected because the order of the messages would become the order the
  threads happened to reach them, and the same input would produce different
  output between runs. Every test suite in this repository is a golden file,
  so non-deterministic diagnostics would make the suites stop proving
  anything. Merging in module order gives byte-identical output on 1 thread
  and on 16.
- `Context::get_logger` currently calls `set_source_file` on every access to
  wire the two together. That disappears: a module owns both.
