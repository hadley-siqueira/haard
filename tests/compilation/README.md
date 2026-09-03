# Compilation tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

**This is the suite that joins the two halves.** Before it, the parser suite
parsed one file and never resolved an import, and the ModuleFinder suite
resolved paths whose `.hd` files were empty markers it never opened. Nothing
tested the cycle the compiler actually runs: read a file, ask what it imports,
resolve each import against the root that file sits in, open what comes back
and parse it too.

Every directory in `cases/` is a project:

```
cases/<name>/
    table.tbl        the roots table, or generated/table.tbl, or neither
    entry            one line: the file to start from
    stop-after-parsing   optional and empty: ask for the parser and no more
    <the sources>    real Haard, parsed by the real parser
```

The golden is the module graph: for each module that was reached, its name, its
file, the imports its source wrote and **what it declares** — that last line is
the proof that the file was not merely found but scanned and parsed.

## What the goldens are actually pinning

**That the loop runs at all.** `two_versions_of_one_library` starts at
`app/main.hd` and ends with eleven modules. Their order is the order they were
reached, so the golden is a breadth-first drawing of the import graph.

**That an imported file belongs to its own root and not to the importer's.**
The same case is the point of strategy B, and the two lines to read are:

```
1 zip.compress  sys/zip/2.0/compress.hd
7 zip.compress  sys/zip/1.0/compress.hd
```

Two modules, one name, two files. The app reached 2.0 and png reached 1.0 from
the identical `import zip.compress`, and each of those then imported
`zip.archive` and stayed inside its own version. Handing the importer's root to
the imported file instead sends zip 1.0's internals into 2.0 and the golden
collapses to one.

That pair is also record 0010's third cost standing in the open: **two types
under one name**. Nothing here complains about it yet, because nothing compares
types yet. When something does, its diagnostic has to name the root of each.

**That a file is one module however many times it is imported.**
`one_file_imported_twice_is_one_module` is a diamond, and `lib.bottom` appears
once. The key is the resolved path, which is what agenda 2.3 asked for and what
makes deduplication fall out instead of being written.

**That the key is canonical and the path is not.** In
`the_entry_file_imported_again` the entry arrives spelled the way the command
line spelled it, and `app/report.hd` imports it back under the canonical path
the finder resolved. One module, not two — and the two spellings are why the
key is canonicalized and the path kept as given: a path is what gets opened, a
key is an identity.

**That modules may import each other.** `modules_may_import_each_other` is
record 0007 as a test. The loop does not detect the cycle, it does not notice
it: the second visit finds the file already interned and appends nothing. Take
the interning out and this case does not fail with a wrong answer, it **hangs**,
which is what the per-case timeout is for.

**That a star import is a directory.** `a_star_import_loads_the_directory` has
`std/net/tcp.hd` below the directory `std/`, and the star loads `io` and `list`
and does not descend. Per record 0006.

**That `--pretty-print` runs nothing after the parser.**
`parsing_only_runs_nothing_after_the_parser` and
`the_same_sources_checked_in_full` are the **same sources twice**, and only the
first writes the marker. Without it the golden is two modules and a name error;
with it, one module, no `depends on` line and no diagnostics — because
following an import and resolving a name are both questions about a program,
and neither is being asked. The pair is the test: either golden alone would
pass a compiler that ignored the switch.

**That a broken module is never asked what it imports.**
`a_broken_module_is_never_asked_what_it_imports` has a dependency whose second
import statement does not parse. The parser drops a failed statement without
taking its nodes back, so its tree is not a tree any reader may walk — and
`AstQuery` assumes a clean parse in every method. The gate is
`Module::is_parsed`, and without it this case loads `lib.fine` out of a module
that never compiled.

Note that `is_parsed` is not `has_errors`. An import that resolved to nothing is
logged **against a module that parsed perfectly**, and that module's tree is
still there to be read — which is why `an_import_that_resolves_to_nothing` still
lists its imports and its declarations under the error.

**That one bad module does not stop the others.**
`a_dependency_that_does_not_parse` keeps `lib.fine` in the graph next to the
broken `lib.broken`. Agenda 4.2 — what happens to an importer whose dependency
failed — is **not decided**, and this golden is deliberately the weakest
possible answer: everything is loaded, everything is reported, nothing is
concluded. When 4.2 lands this case changes, and that is the point of having it
written down.

## The `depends on` lines, and what they add

The `imports` lines say what the **source wrote**; the `depends on` lines say
what the compilation **resolved it to**, as module indices in the order record
0009 reads them — the order of the imports in the file, with a star import's
files sorted inside it.

They pin things the module list alone could not:

- **The edges of the graph, and not only its nodes.** In
  `modules_may_import_each_other` the two lines `depends on 1 lib.b` and
  `depends on 0 lib.a` are the cycle drawn as edges. The module list has always
  shown both files; nothing showed that each points at the other.
- **Strategy B, structurally.** In `two_versions_of_one_library`, module 1 is
  zip 2.0 and depends on 5 and 6, module 7 is zip 1.0 and depends on 9 and 10.
  Each version's internal imports stayed inside its own root, and now that is
  an explicit edge instead of something inferred from the order files were
  reached.
- **That a star's expansion is sorted**, in `a_star_import_loads_the_directory`:
  `std.io` before `std.list`, which record 0006 requires and record 0009 made
  decide which declaration a bare name means.
- **That a diamond is one module and two edges.** In
  `one_file_imported_twice_is_one_module`, modules 1 and 2 both say
  `depends on 3`.

**Two sabotages, both caught, and neither caught by anything else in the
suite:**

- resolving a module's imports in reverse source order — 3 cases fail. This is
  the property record 0009 rests on: first-import-wins is first *in the file*.
- recording every dependency on the entry module instead of on the importer —
  4 cases fail. Every other golden here still passed, because the set of
  modules reached is identical and only the edges moved.

## The two ways an import fails, in a real diagnostic

`an_import_with_no_entry_in_the_table` and `an_import_that_resolves_to_nothing`
are the finder's two failure statuses turned into rustc-shaped errors pointing
at the import path:

```
error: the root 'app' has no entry named 'png'
 --> app/main.hd:1:8
  |
1 | import png.encode
  |        ^^^^^^^^^^
```

The underline covers `png.encode` and not the `import` keyword, which is the
same six characters in every file and says nothing about which import went
wrong. That span is `AstQuery::get_import_offset` and `get_import_length`.

The first message is about the **table**, the second about the **source**, and
the second shows the path it looked at — with two versions of a library in one
program, the author cannot tell which one he reached without being told.

## Two cases about the edges

`no_table_means_one_file` has no table: the entry is compiled, its import is
not followed and not complained about, and the module has no name because it is
in no root. That is `hdc file.hd` as it has always behaved, and it must keep
working.

`the_entry_is_under_no_root` is the other edge: a table exists and does not
cover the file it was asked to start from. That is not a diagnostic — there is
no source position to point at — so it is a message and exit code 2, like a bad
command line.

## The line at the bottom of every golden

```
--- parsed alone: every module matches
```

Every tree the loop built is compared against the tree the same file gives when
it is the only thing parsed. It is the one property a suite of single files
cannot have, because it is about what happens to the fifth file *because* four
were parsed before it: `Compilation::load` builds a `Scanner` and a `Parser` per
module, and hoisting them out of the loop is the obvious optimization.

**Be honest about what this currently proves.** Two sabotages were tried and
neither made it fire:

- hoisting the `Scanner` and the `Parser` out of the loop — `get_tokens` calls
  `reset()` and `parse()` clears every field it owns including the indentation
  stack, so there is nothing to leak;
- the same, with `parse()`'s indentation-stack reset removed — `parse_block`
  pops what it pushes, so the stack is back at `[0]` by the end of a file
  anyway.

It does fire when a module's tree genuinely differs — parsing the wrong file
proves that — but so do the module-graph goldens above it. **No sabotage was
found that only this line catches.** It is kept as a guard on a contract that
currently holds and is invisible: the resets in `Scanner::get_tokens` and
`Parser::parse` are what make the loop safe, and nothing else in the repository
says so.

What it can catch that the goldens above cannot is a leak that mangles the
*inside* of a function body while leaving its name alone: the goldens compare
what each module declares, and this compares the whole tree. That gap is real
but was not demonstrated with a sabotage, so read this line as a guard and not
as evidence.

## Reading the golden diff

**`./run.sh -u` accepts whatever the compilation produces, bugs included.**
Reading `git diff expected/` afterwards is the step that gives the suite its
value. Four of these goldens were wrong when first written: two fixtures had a
`class Archive` missing its colon, the diagnostics carried the machine's home
directory, and a module with an unresolved import was labelled as not having
compiled when its tree was perfectly good.
