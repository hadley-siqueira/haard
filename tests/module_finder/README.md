# ModuleFinder tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is one table and the tree it resolves into:

```
cases/<name>/
    table.tbl        the roots table, or generated/table.tbl when the case
                     needs paths that climb out of the table's own directory
    queries.txt      the commands find.cpp runs
    <a tree>         whatever the queries resolve into
```

`find.cpp` runs the queries and writes the answers to `expected/<name>.txt`.
**Every path in the output is printed relative to the case directory** — an
absolute one would carry this machine's home into the goldens and the suite
would pass only here.

## What the goldens are actually pinning

**Strategy B itself.** `two_versions_of_one_library` is `exemplo/` in miniature:
`app/main.hd` and `sys/png/1.0/encode.hd` write the same `import zip.compress`
and reach `sys/zip/2.0/compress.hd` and `sys/zip/1.0/compress.hd`. It is the one
case that would be impossible with a single global table.

**That a library importing itself lands in its own root.** The same case ends
with `sys/zip/1.0/compress.hd` and `sys/zip/2.0/compress.hd` both writing
`import zip.utils` and staying home. This is the `= name` on the header line,
and dropping it silently sends zip 1.0 into zip 2.0's files — a library
compiling against another version of itself.

**That the prelude is a block with no directory.** Record 0033 makes the
prelude a list of imports every module of the program is given, and
`the_prelude_is_a_list_of_imports` pins the three things about it that could
each be got wrong on their own: it is a `Root` whose path is empty, its entries
are **resolved when the table is read** and not per importing module, and
`file loose/stray.hd -> no root` is the one guard that empty path needs — a
prefix test against nothing matches every file, so without it the prelude
claims every file no real root covers.

The case holds two blocks named `zip` for a reason: it is why a prelude entry
cannot be resolved by looking a root up by name. Block names are not unique,
so the prelude has a visibility list like anybody's, and `import std.string`
inside it goes through `find` unchanged.

The four error cases are the table being wrong, once, before a module is
loaded: a second `prelude` block, an `import` line in a root block, an entry
whose first segment the prelude cannot see, and an entry with no file at the
end of it.

**That the table is read in two passes.** `roots_may_see_each_other` has `a`
depending on `b` and `b` on `a`, so a dependency must be allowed to name a
block that has not been read yet. Resolving them as they are read fails this
case, and the second pass is where a dependency pointing at no block is caught
(`a_dependency_names_no_block`) — that check is free because the pass turns a
path into a block index anyway.

**That a prefix is compared component by component.**
`a_prefix_is_not_a_string` has the roots `lib/1.0` and `lib/1.0-beta`, and
`lib/1.0-beta/a.hd` belongs to the second. On a string compare the first root
is a prefix of that file, both roots are the same depth, and the file goes to
the wrong one without a word.

**That the longest prefix wins, not the first.** `longest_prefix_wins` nests
`tree/inner` inside `tree`. Roots nest in practice — `sys/zip/1.0` sits under
`sys` — so the outer root must not capture the inner root's files.

**That one segment does double duty, and that a star does not.**
`single_segment_import` asks for both against the same name: `import foobar` is
`foobar/foobar.hd`, and `import foobar.*` is the directory `foobar/`. It is the
only place the two rules part, and giving the star the file rule empties every
star import of a root. The same case pins the inverse: `foobar/foobar.hd` is the
module `foobar` and not `foobar.foobar`.

**That a star import is one directory, sorted.** `star_is_one_directory` puts a
`net/tcp.hd` below `std/` and a `README.md` beside `io.hd`: neither shows up.
The sort is not cosmetic — `directory_iterator` has no order of its own and
record 0006 hashes this expansion, so an unsorted list moves the cache key
between machines. Be warned that this is the one golden here that could pass
with the bug present, if the filesystem happens to hand the entries back in
order; it caught the sabotage on ext4 and that is not a guarantee.

**That a relative path in the table resolves against the table.**
`paths_are_relative_to_the_table` keeps its table in `generated/` and points at
`../sys` and `../app`. Resolving against the working directory instead breaks
almost every case here, because the suite runs from `tests/module_finder` and
the tables live two levels down — which is exactly the accident the rule
exists to prevent: the same command from two directories resolving to two
different files, and the cache key of record 3.4 moving with it.

## The two ways a lookup fails, and why they are separate

`the_file_is_missing` and `no_entry_for_the_first_segment` are the same question
answered by two different people:

- **no entry** is an error in the *table*. The block simply does not list that
  name, and the message can say which entry is missing from which root.
- **nothing there** is an error in the *source*, and the golden shows the path
  it looked at. Without that path the author cannot tell which version of the
  library he actually reached, which is the failure mode strategy B is most
  likely to produce.

## The malformed tables

Five cases, one line of table each: a dependency before any block, a line that
opens no block, two blocks for one directory, a name listed twice in one block,
and no table at all. Their goldens are the messages, and they are goldens
precisely because the messages are the product — a table is generated by
something else, so when it is wrong the reader is looking at output he did not
write.

`two_blocks_for_one_root` spells the second block `./lib/` against the first's
`lib`, so it also pins that the paths are compared after normalization and not
as text.

## Reading the golden diff

**`./run.sh -u` accepts whatever `ModuleFinder` produces, bugs included.** The
step that gives the suite its value is reading `git diff expected/` afterwards.
One golden here was wrong the first time it was written — a `name` query ran
under the wrong root and produced `app....foobar.foobar`, which is what a file
outside its root looks like. The fix was in the query, and the answer is worth
knowing: `module_name_of_file` trusts that the file is inside the root, because
the only thing that produces the root is `root_of_file` on that same file.

The per-case `timeout` is a test of its own: a table read that stops making
progress shows up as a failure instead of hanging the suite.
