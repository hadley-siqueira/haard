# The 1.9 proposal, next to npm, Cargo and the rest

Background note for agenda item 1.9, written 2026-09-01 while the proposal was
still a proposal. It was confirmed the same day —
`../0010-a-table-per-root.md` is the record, and this note is kept as the
comparison that argued for it. Companion to `how-other-languages-resolve-imports.md`, which
covers the compilers; this one compares the **whole path from what a user types
to what the compiler is handed**, and is honest about what the proposal costs.

The proposal, in one line: a **per-root table** generated from per-library
manifests, handed to `hdc`, which searches nothing. Worked example in
[what-a-package-manifest-would-hold.md](what-a-package-manifest-would-hold.md),
and in `tests/compilation/cases/two_versions_of_one_library` as a case.

## The five stages, side by side

| | what the user edits | how a version is picked | where it lands on disk | what the compiler gets | two versions at once |
|---|---|---|---|---|---|
| **npm** | `package.json`, ranges | solver + `package-lock.json` | `node_modules` tree, **shape = resolution** | nothing; the runtime walks up | yes, by nesting |
| **pnpm** | `package.json`, ranges | solver + lockfile | global CAS, hardlinks, symlink tree | nothing; the walk is fenced to declared deps | yes |
| **Yarn PnP** | `package.json`, ranges | solver + lockfile | zips | **`.pnp.cjs`: a literal table** | yes |
| **Cargo** | `Cargo.toml`, ranges | solver + `Cargo.lock` | `~/.cargo/registry` | **`--extern name=path`, per crate** | yes, semver-incompatible only |
| **Go** | `go.mod`, exact | MVS, no solver | `$GOMODCACHE/mod@version` | exact dirs, longest prefix | one per major, name carries it |
| **Zig** | `build.zig.zon`, **content hash** | none — the hash is the identity | `~/.cache/zig/p/<hash>` | **`-M name=path` + `--dep`** | yes, dedup is manual |
| **Maven** | `pom.xml`, ranges | solver, then **flattened** | `~/.m2/repository` | ordered classpath | no |
| **pip** | `requirements`, ranges | solver, then flat install | `site-packages` | `sys.path` | no |
| **CMake/Conan** | varies | varies | varies | `-I` / `-L` flags | no |
| **Haard (proposta)** | `haard.pkg`, one per library | deferred — the table has no versions | anything; the table holds paths | **`roots.tbl`: per-root table** | yes |

The proposal sits in the same square as Yarn PnP, Cargo and Zig: the resolver
finishes its job completely, and the compiler is handed a table with no
searching left in it.

## Where the proposal differs from each of them

**Against npm.** Same capability, different carrier. npm expresses per-importer
scoping as **directory nesting**, and pays with phantom dependencies (hoisting
makes an undeclared package importable), invisible duplicate instances, and a
huge tree that has to be materialized before anything compiles. The proposal
expresses it as a table, which is where npm itself is heading — Yarn PnP is
exactly this move. The proposal starts where npm arrived.

**Against Cargo.** Nearly the same shape. Two differences worth naming:

- Cargo's table is per *crate* and the crate is the unit of compilation; the
  proposal's is per *root* and the unit of compilation is a module, or a group
  of modules in a cycle (record 0007). Finer grain, same mechanism.
- Cargo unifies semver-compatible versions automatically, so duplicates only
  appear across incompatible majors. The proposal has no solver yet, so
  **nothing unifies anything** — whatever the generator writes is what happens.
  That is fine while there are no third-party libraries and is the first thing
  a package manager will have to add.

**Against Go.** Go refuses the whole problem: one version per major, and the
major goes *in the import path*, visible in the source. It is the simplest
design on the table and the one users praise most. The proposal is strictly more
capable and strictly more machinery. The Go answer stays available as a
*policy*: a generator that writes the same root for every consumer produces
exactly Go's behaviour.

**Against Zig.** The closest relative. Zig's module root fence — a path import
may not escape the module's root directory — is the same guarantee the proposal
gets from the first segment always being a table lookup. Zig's content hash as
the entire identity, with no versions and no solver, is worth remembering when
agenda 3.4 is written.

**Against Maven and pip.** These are what the proposal is designed not to be.
Both resolve properly and then **throw the structure away** at the handoff, so
first-match-wins over a flat namespace decides what you got, and no diagnostic
exists for the wrong answer.

**Against CMake.** The motivating case. A C++ repository assumes libraries are
already installed on the system, in the right version, in a place an ordered
`-I` list happens to reach. Nothing in the repository can guarantee it and the
failure is `foo.h: No such file or directory`, which does not say where it was
supposed to come from. Every one of those three properties is inverted here.

## What `git clone` needs, per ecosystem

| | what must already be on the machine | what the failure looks like |
|---|---|---|
| C++ / CMake | the libraries, correct versions, discoverable | `foo.h: No such file or directory` |
| Maven | a JDK; jars are fetched | a resolution conflict, or a silent classpath shadow |
| pip | a Python; wheels are fetched | a resolution conflict, or the wrong `foo` on `sys.path` |
| npm / Cargo / Go / Zig | the toolchain only | rare; the resolver reports it before compiling |
| Haard (proposta) | the toolchain only, **once a package manager exists** | an unresolved import naming the missing table entry |

The last row's caveat is the honest part: the property is bought by the package
manager, not by the compiler. What the compiler decision buys is that the
package manager *can* deliver it — a compiler that searches ambient paths cannot
be rescued by any resolver, which is C++'s whole situation.

## What the proposal costs

Six things, ordered by how likely each is to actually bite.

1. **It needs a generator to be good.** Until a build system writes `roots.tbl`,
   somebody writes it by hand — and a hand-maintained table that grows with the
   dependency graph is the `CMakeLists.txt` this design exists to escape. The
   mitigation is that the generator is simple precisely because there is no
   solver yet: read the manifests, walk the graph, write the blocks.
2. **Two types with one name.** Two roots of the same library declare two
   `zip.archive.Archive`s, and passing one where the other is expected must be
   an error. If the diagnostic does not name the root of each, it reads as
   `expected Archive, found Archive` — the most confusing message Cargo
   produces. Naming the root costs one line and must not be skipped.
3. **Duplicate global state.** Haard has module-level `let` and `const`, so two
   versions of a library are two copies of its globals. This is Node's duplicate
   instance problem, where two copies of one module hold two states and
   `instanceof` across them silently fails. Nothing warns unless something is
   built to warn.
4. **Duplication costs binary size and compile time.** Real in npm and Cargo,
   and the reason both have tooling to list duplicates (`cargo tree -d`).
5. **The table is O(roots x dependencies) and repetitive.** Ugly, generated,
   never read by a user — the cheapest of the six.
6. **Deduplication is not automatic, but it is nearly free.** If two blocks name
   the same path, the module registry keyed by resolved path (agenda 2.3) gives
   one module, compiled once. Dedup therefore falls out of the cache key rather
   than needing a rule of its own — provided the generator writes identical
   paths for what it intends to share.

## What the proposal buys

1. **The compiler is told and never searches**, so no ordering can silently
   change what an import means and an unresolved import is one exact message.
2. **A root is a fence.** `import zip.…` inside `zip` can never name a file
   outside `zip`'s root. This is Zig's rule and it is what makes the whole
   scheme safe.
3. **Two versions coexist without nesting directories (npm) and without renaming
   packages (Java shading).**
4. **The user edits four lines**, and never sees the table.
5. **No JSON parser in the bootstrap.** A line-oriented table is about thirty
   lines of code, which matters under a no-external-dependencies rule.
6. **Versions never reach the compiler**, so version resolution can be designed
   later without touching the resolver — Zig's content hash and Go's MVS both
   stay reachable.

## The one-sentence version

npm's capability, Cargo's and Yarn PnP's carrier, Zig's fence, and Go's
simplicity still available as a policy on top — at the cost of needing a
generator, and of owing a good diagnostic for the two-types-one-name case.
