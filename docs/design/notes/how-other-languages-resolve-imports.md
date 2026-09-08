# How other languages turn an import into a file

Background note for agenda items 1.3, 1.6, 1.7, 1.8 and 1.9. Not a record: it
decides nothing, it collects what the languages that already answered these
questions actually do, and what each answer cost them.

Records [0001](../0001-module-path-to-file.md) and
[0006](../0006-star-import-is-a-directory.md) already put Haard in one corner of
this space — a module path is a file path, a star import is a directory listing.
Most of what follows is therefore read for its *consequences* rather than for
its mapping: the mapping is settled, the failure modes are not.

It is in two halves, because the question has two layers. The first half is what
the **compiler** does with a specifier. The second half — *The layer above* — is
what the **package manager** does before the compiler runs, since the specifier
in the source never carries a version and something has to supply one. The seam
between those two layers is where most of the interesting damage happens.

## The six axes that actually separate them

Every language below can be placed by answering six questions. Reading the
survey with these in hand is much shorter than reading it language by language.

**A. What does the import name?**
A *file or directory* (Go, Node, Python, Zig, OCaml, Haard), or a *symbol /
namespace* whose relationship to files is a separate convention (Java, C#,
Rust's `use`). This is record 0001's question, and it is the one that decides
whether the resolver opens a file or looks inside one.

**B. Who owns the name-to-path mapping?**
The *language* (Python, Node, Go, Java, Haard), or the *build system* (Rust,
Zig, C++20 modules, C#). Handing it to the build system makes the compiler
deterministic and the ecosystem fragmented — it works for Rust because Cargo is
effectively the only build system, and it failed for C++ because there are
twenty.

**C. Is the search path ambient or explicit?**
A `PATH`-style list searched at compile time (C's `-I`, Java's classpath,
Python's `sys.path`, OCaml's `-I`), or an explicit table of every dependency
mapped to an exact path (Rust's `--extern`, Zig's `-M`, Go's module cache
resolved through `go.mod`). Ambient means first-match-wins and shadowing;
explicit means a manifest and no surprises.

**D. Does the answer depend on who is importing?**
Node's `node_modules` walk and C's quoted `#include` both resolve relative to
the *importing* file, so one specifier can mean different files in one program.
Java, Go, Python (module imports) and Rust do not.

**E. Is an import a load with side effects, or a pure name binding?**
Python and Node *execute* the imported module; Java, Rust, Go and C# only bind
a name. This decides what a cycle even means.

**F. How is the compiled interface cached, and what is in the key?**
The part that matters for the `.hdm`.

---

## C and C++ — `#include`

**The mapping.** There isn't one, in the sense that matters here: `#include`
names a *path fragment*, not a module, and the result is textual substitution.

- `#include <foo/bar.h>` searches only the configured directory list:
  `-I` entries in order, then `-isystem`, then the built-in system directories.
- `#include "foo/bar.h"` is implementation-defined; in practice every compiler
  searches **the directory of the including file first**, then falls through to
  the `<>` list.

First match wins, exactly like `PATH`. There is no notion of the "right" file
for a given name.

**What it costs.**

- **Shadowing is silent and normal.** A local `list.h` earlier on the include
  path than the system one is not an error, it is the documented mechanism. The
  build is a function of the `-I` order, which lives in a makefile, which is why
  "works on my machine" is a C++ genre.
- **The same header is re-parsed once per translation unit.** Include guards
  and `#pragma once` stop the *text* being processed twice within one TU; they
  do nothing across TUs. This is the specific cost Haard's `.hdm` exists to
  avoid, and it is worth being precise that the waste is re-parsing, not
  re-reading.
- **`#pragma once` keys on the file, not the name.** Two paths to the same file
  (a symlink, a `-I` that reaches it by a different route, a bind mount) can
  defeat it. Compilers work around it by canonicalizing, which is not free.
- **Order dependence.** A header that only compiles when something else was
  included first is legal, so the meaning of a file depends on its context. This
  is what modules were introduced to kill.

**C++20 modules — the cautionary tale worth the most.** `import foo.bar;` looks
like the thing Haard is building. The standard deliberately specifies **no
mapping from module name to file**: the module name is an opaque sequence of
identifiers with no required relationship to any path. Consequences:

- Every toolchain invented its own. GCC has a *module mapper* (a file, or a
  program speaking a protocol, via `-fmodule-mapper=`); Clang wants explicit
  `-fmodule-file=name=path`; MSVC emits `.ifc` and takes `/reference name=path`.
- The build system must discover the module dependency graph *before* compiling,
  which requires scanning sources for `import` lines — a whole extra tool
  (`clang-scan-deps`, and a standardized JSON format, P1689) exists for this.
- Binary module interfaces are **not portable across flags**: the same source
  with different `-D` or `-std` produces a different BMI, and using a mismatched
  one is undefined rather than diagnosed. This is exactly the "compiler version,
  format version and flags in the cache key" requirement, learned the hard way.

The lesson for Haard is not about the mapping — record 0001 already refused this
particular freedom — it is that **leaving the mapping unspecified moves it into
the build system, and then there is no build system**.

---

## Java

**The mapping.** `package a.b.c;` and `import a.b.c.Foo;`. The import names a
**type**, not a file (axis A: symbol). The binary name `a.b.c.Foo` maps to the
path `a/b/c/Foo.class` — inside a *classpath entry*, which is either a directory
or a `.jar` (a zip, looked up by entry name). The classpath is an ordered list;
first match wins.

Two rules make the mapping work despite the import naming a type:

- **One public top-level class per file, and the file name must equal the class
  name.** So the source mapping `a/b/c/Foo.java` is enforced by the compiler,
  not left to convention.
- Package names lowercase, type names CamelCase — convention only, but it is
  what keeps `a.b.C.d` unambiguous to a human.

**An import binds nothing that a fully qualified name cannot say.** This is
axis E, and Java is the clean example: `import` is a per-compilation-unit
abbreviation. `new a.b.c.Foo()` works with no import at all. Loading happens
when the class is *first used*, by the class loader, at run time.

**Star imports are on-demand, and this is where Java answers item 1.7.**

- `import a.b.*` imports the types of package `a.b`, **not** its subpackages.
- A single-type import beats an on-demand import; a type in the current package
  beats an on-demand import.
- If two on-demand imports both supply `List`, that is **not** an error at the
  import — it is an error at the *use*, and only if the ambiguous name is
  actually used. Adding a class to a library can therefore break a downstream
  file that compiled yesterday. This is a real and recurring cost, and it is the
  precise shape of Haard's item 1.7.

**JPMS (Java 9+)** added a second, separate namespace: modules declared in
`module-info.java` with `requires` and `exports`, found on the *module path*
rather than the classpath. Notably it forbids **split packages** (two modules
contributing types to one package), because the classpath's first-match-wins
made split packages a silent-wrong-answer machine. Haard's "a module is a file"
rule makes split packages impossible by construction, which is a real advantage
to notice.

**Caching.** `.class` files, with a constant pool and a version stamp; the
compiler reads a dependency's `.class` rather than re-parsing its `.java`. Same
shape as the `.hdm`, decades earlier.

---

## Python

The most permissive system here, and the most instructive about what
permissiveness costs.

**The mapping.** `import a.b.c` imports `a`, then `a.b`, then `a.b.c`, each of
which must be a module or a package. `sys.path` is an ordered list of
directories (and zip files); first match wins.

- A **package** is a directory. Classically one containing `__init__.py`.
- **PEP 420 namespace packages**: a directory *without* `__init__.py` becomes a
  namespace package whose contents are the **union of that directory across
  every `sys.path` entry**. This is directly the star-across-roots question in
  the ModuleFinder discussion, answered as "union" — and the price is that a
  package's contents depend on the whole search path, and a stray empty
  directory anywhere on `sys.path` can hijack a top-level name.

**`from a import b` is genuinely ambiguous, and Python resolves it at run
time.** `b` may be a submodule `a/b.py` or an attribute of `a`. The
implementation tries the attribute, and on failure attempts to import the
submodule. This is the same ambiguity record 0001 settled statically for Haard;
worth knowing that Python's answer only works *because* it is dynamic, and is
therefore not available.

**`sys.path[0]` is the script's directory.** So the entry file's own directory
does take part in the search — and it is one of the best known foot-guns in the
language: naming your file `random.py` breaks the standard library for the whole
program, with an error message that points somewhere else entirely. Item 1.9's
"does the current file's directory take part" has an empirical answer here, and
it is *no*.

**Explicit relative imports** (`from . import x`, `from ..pkg import y`) exist
and are resolved from `__package__`, not from the file's location on disk.
Python 3 **removed** implicit relative imports precisely because "is `string` my
sibling or the stdlib?" could not be answered by reading the file.

**Imports execute code, and `sys.modules` is the cache** (axis E). A module runs
exactly once per interpreter, keyed by its dotted name. Consequences:

- **Circular imports half-work**: the second import gets the partially
  initialized module object. Whether it breaks depends on whether the needed
  name was bound before the cycle re-entered. Item 1.3's "supported" option, in
  its worst form — supported by accident rather than by design.
- Keying the cache by *name* rather than by *path* means the same file reached
  by two names (`pkg.mod` and `mod`) is two module objects with two copies of
  its globals.

**`from a import *`** takes the module's *names*, filtered by `__all__` or by
the leading-underscore rule. For a package it does **not** pull in submodules
unless `__init__.py` says so — which is a different meaning from record 0006's,
and the mismatch between the two readings is exactly why 1.8 is still open.

**Caching.** `__pycache__/mod.cpython-313.pyc`, with the interpreter version in
the *filename* (PEP 3147) and a validation stamp in the header: either source
mtime+size, or, since PEP 552, a hash of the source. The magic number changes
whenever the bytecode format does. Three ideas Haard already plans to copy —
version in the key, hash-based invalidation, reject rather than port a foreign
cache.

---

## Node.js — CommonJS

**The mapping.** `require(specifier)`, resolved **relative to the requiring
file** (axis D). Two cases:

- *Relative or absolute* (`./foo`, `../lib/foo`, `/abs/foo`): try the path as a
  file; then with `.js`, `.json`, `.node` appended; then as a directory — its
  `package.json` `"main"`, else `index.js`.
- *Bare* (`lodash`): walk **up** the directory tree from the requiring file,
  trying `node_modules/lodash` in each ancestor, up to the filesystem root.

So the search path is not configured at all: it is *derived from the importer's
position in the tree*.

**What it costs.** This is the strongest argument in the whole survey for item
1.9's third question.

- **One specifier, several files, one program.** Two packages depending on
  different versions of the same library get their own nested `node_modules`
  copy. That is the feature. The consequence is that a library can be loaded
  twice, with two copies of its module state, and `instanceof` across the two
  fails; every ecosystem with a singleton (React hooks, a database driver's
  connection pool) has a dedicated error message for it.
- **`require.cache` is keyed by resolved absolute path**, so identity is
  per-file, not per-name — which is what makes the duplicate instances distinct
  rather than shared.
- **Extension guessing is a resolution search.** It costs several failed
  `stat` calls per import and makes the meaning of `./foo` depend on which
  files happen to exist next to it.
- Circular `require` returns the partially populated `module.exports` — the same
  half-working cycle as Python.

## Node.js and browsers — ESM

The redesign, and it moved almost every dial:

- **Specifiers are URLs**, resolved against the importing module's URL. In the
  browser there is no search at all: `./foo.js` is a URL join, and a bare
  specifier `lodash` is a **hard error** unless an **import map** supplies a
  mapping. The mapping became explicit data (axis C), supplied by the host.
- **No extension guessing** for relative specifiers — the path is the path.
- **`"exports"` in `package.json`** replaced "any file in the package is
  reachable" with a declared entry-point map. A package can finally have private
  files. `"imports"` does the same inward, mapping `#internal` specifiers.
- Imports are **statically analyzable**: the specifier is a string literal in
  the grammar, so the dependency graph can be built without executing anything —
  which is what makes bundling and tree-shaking possible, and what
  `require(expr)` prevented.
- Cycles are handled by **hoisted, uninitialized bindings** (a temporal dead
  zone) rather than by half-built objects: the cycle is diagnosed as an access
  before initialization, at the exact point of use.

Deno's variant is worth one line: specifiers are literally remote URLs, cached
locally in a content-addressed directory and pinned by a lockfile — the
strongest possible form of "the name is the location".

---

## Rust

**The mapping is in the source, and it is a declaration rather than a
discovery.** Two separate mechanisms, and keeping them apart is the whole point:

1. **`mod foo;`** tells the compiler to load `foo.rs`, or `foo/mod.rs` (2015
   style) — a file in the directory that no `mod` declaration names is simply
   **not part of the crate**. There is no directory scan. Adding a file changes
   nothing until someone declares it.
2. **`use path::to::Thing;`** loads nothing whatsoever. It is a name alias
   inside the current scope, resolved against the already-known module tree:
   `crate::`, `self::`, `super::`, or an external crate's name. Any `use` can
   be replaced by writing the path in full.

**External crates are passed in explicitly.** Cargo invokes rustc with
`--extern serde=/path/to/libserde-<hash>.rlib` for each dependency. The compiler
does not search for a crate by name in the ordinary case: version resolution,
the registry cache under `~/.cargo/registry`, and the lockfile are Cargo's job
(axis B and C: build system, explicit). The result is that **no `-L` ordering
can silently change which crate you got**.

**Glob imports and ambiguity** (item 1.7): `use foo::*;` is allowed, and
glob-imported names sit at a lower precedence than explicitly imported or
locally defined ones, so a new item in `foo` shadowed by your own definition is
not an error. An ambiguity between *two* globs is an error **at the use site**,
like Java — but the shadowing rule removes most of the cases where Java breaks.

**Cycles**: forbidden between crates, unrestricted between modules of one crate.
That split is the clean version of item 1.3 — the unit that can be compiled and
serialized independently is the unit that may not participate in a cycle.

**Caching.** `.rlib`/`.rmeta`, with `-C metadata` folding the crate version,
features and relevant flags into a hash that appears in the filename, so two
configurations coexist in one directory instead of clobbering each other. Cargo
adds its own fingerprint files. Same requirement as agenda 3.4, with a detail
worth stealing: **putting the key in the filename** means a stale artifact is
never *found*, rather than being found and rejected.

---

## Zig

Closest in spirit to what Haard is building, and it splits the problem in two:

- **`@import("std")`** — a bare name is a *module* declared on the command line
  (`-M`/`--dep` in recent versions, wired up by `build.zig`). The compiler does
  not search for it.
- **`@import("foo/bar.zig")`** — a path is resolved **relative to the importing
  file**, and is not allowed to escape the module's root directory. The root
  is a boundary: a module's files are exactly the ones under it.

So relative-to-the-importer resolution exists (axis D) but is fenced by the
module root, which removes Node's failure mode: a path import can never reach
another module's internals, and can never mean two different files.

Other points worth taking:

- **A file is a type.** `@import` of the same resolved path twice yields the
  same struct type, so identity is by canonical path, decided once.
- **There is no glob import**, and no wildcard of any kind.
- Package dependencies live in `build.zig.zon` and are fetched into a global
  cache **keyed by a hash of their content**, not by a name and a version.
- Analysis is lazy, so mutually importing files are ordinarily fine — cycles
  are a problem only when they are a genuine cycle in the *values* being
  computed, and then they are diagnosed as such.

---

## Go

The most direct precedent for "the import path is a directory".

**The mapping.** `import "github.com/user/repo/net/http"` names a **directory**.
Every `.go` file in that directory is the package — **no per-file declaration**,
unlike Rust's `mod`, and no wildcard needed, unlike record 0006's star. Adding a
file to the directory adds it to the package.

The resolution: the longest matching **module path** prefix from `go.mod` is
looked up (in the module cache `$GOMODCACHE/<module>@<version>/`, populated from
the network and verified against `go.sum`), and the remainder is a relative
directory under it. Standard library paths are the ones whose first element
contains no dot.

Points worth taking:

- **The package's name and its directory's name are independent.** The `package`
  clause supplies the identifier used in code; the import path supplies the
  location. They match by convention, and an import may alias explicitly. This
  is the cheap way to keep item 1.6 flexible without making the path ambiguous.
- **Import cycles between packages are a hard error**, and this is what buys Go
  its compilation model: a package's exported interface is written once to a
  compiled archive and every importer reads that, never the sources. The
  "compile once, publish a blob, everyone consumes the blob" design in
  the module/compilation design is Go's design, and it depends on the
  acyclicity.
- **Unused imports are a compile error**, which is only possible because an
  import is a pure binding with no side effect.
- The pre-modules era (`GOPATH`, where the import path was appended to a single
  root) was abandoned largely because one global root could not hold two
  versions of anything. That is the argument *against* record 0001's single
  environment-variable root, and *for* item 1.9's list — but note Go's
  replacement was not a longer list, it was a **manifest**.

---

## Haskell — GHC and Cabal

**The mapping.** `import Data.Map` names a **module**, and a module name maps to
`Data/Map.hs` under one of the `-i` source directories, or — for anything
already compiled — is found in a **package database**. The module namespace is
flat within a package: there is no dotted path to a directory, the dots are just
part of the name.

**The handoff is Family 1 and it is unusually precise.** Cabal resolves the plan
and invokes GHC with `-package-id <name>-<version>-<hash>` for each dependency.
That hash — the *unit id* — folds in the package name, its version, **and the
unit ids of its own dependencies**, so two builds of one package against
different dependencies are different units with different identities.

**The mechanism is per-importer, and the policy is not.** Cabal passes a
different `-package-id` set when compiling each package, so GHC itself has no
global name table at all — nothing stops `png` being compiled against
`zip-1.0-abc` while `app` is compiled against `zip-2.0-def`. What stops it is
**Cabal's solver**, which requires a consistent plan per component and reports a
conflict instead. So Haskell is a language whose *compiler* is B and whose
*build system* chooses A. The historical cost of that policy has its own name,
"Cabal hell".

**Same module name from two packages** is an ambiguity reported at the import,
and the escape is the `PackageImports` extension: `import "containers" Data.Map`
names the package inline.

---

## C# — the assembly, not the file

**The mapping.** `using System.Collections;` names a **namespace**, which has no
required relationship to any file or to any assembly: one assembly may hold many
namespaces and one namespace may be spread over many assemblies. Nothing is
searched by name — the compiler is handed `/reference:/path/to/lib.dll` for each
assembly, one list per project. Family 1.

**Per-project, therefore per-importer.** Each project compiles with its own
`/reference` set, so the machinery is B for the same reason Haskell's is. In
practice NuGet unifies a transitive graph to **one version per assembly name**
before generating those references, which is an A-shaped policy over a B-shaped
mechanism — the same split as Haskell.

- .NET Framework's GAC did allow genuine side-by-side by *strong name*
  (name + version + public key), which is the "how many names, how many
  versions" generalization again.
- When two referenced assemblies really do collide, the escape is
  **`extern alias`**: `/reference:OldZip=old.dll` and then `extern alias OldZip;`
  in the source. A per-import rename, like Haskell's `PackageImports`.

---

## Two smaller ones that are unusually close to Haard

**OCaml.** A module is a file: `foo.ml` defines module `Foo` (the basename,
capitalized) — the mapping is even tighter than Haard's, since there is no
dotted path, just a flat namespace per include directory. `-I` gives an ordered
search list. Compiling `foo.ml` emits `foo.cmi`, the *interface*, and an
importer reads only the `.cmi`. The compiler refuses cycles between compilation
units, and it detects a stale `.cmi` by a hash of the interface stored in every
file that depends on it — an `interface_hash` in the sense of agenda 3.4, in
production since the nineties. The flat namespace turned out to be the mistake:
every library had to prefix its module names by hand until "wrapped" libraries
and module aliases were bolted on.

**Ada / GNAT.** `with Foo.Bar;` maps to the file `foo-bar.ads` — dots become
hyphens rather than directory separators, so the whole hierarchy is flat within
a source directory, found via an ordered source path or a project file. Ada also
has an explicit elaboration order and forbids circular dependencies between
specifications, and it is one of the few languages where the "what happens to an
importer when a dependency fails" question (item 4.2) has a formal answer.

---

## The layer above: manifests, lockfiles and stores

Everything so far described what the *compiler* does with a specifier. In every
modern ecosystem there is a second layer underneath it, and the specifier in the
source usually says nothing about it. `import lodash` contains no version.
`use serde::Serialize` contains no version. `#include <zlib.h>` contains no
version. The version is supplied by something else, and **where the seam falls
between that something else and the compiler is the design decision.**

### Five things, and the question of who knows which

1. **The specifier** — what is written in the source. `lodash`, `std.io`,
   `github.com/x/y/net/http`.
2. **The requirement** — what the manifest asks for. Usually a *range*
   (`^4.17.0`, `"1.0"`), sometimes an exact version (Go), sometimes a content
   hash (Zig).
3. **The resolution** — the exact version the solver picked, plus an integrity
   hash, written to a lockfile so the next machine gets the same answer.
4. **The location** — where that exact artifact sits on disk: a store layout.
5. **The handoff** — what the compiler is actually given.

Step 5 is the one that decides the failure modes, and there are only two
answers to it.

### Family 1 — the build system resolves, then hands the compiler an exact table

Rust, Zig, C#, Go, and Yarn's PnP. The package manager finishes its job
*completely* and gives the compiler a name-to-path mapping with no searching
left to do.

Cargo is the clearest. `Cargo.toml` says `serde = "1.0"`; the solver picks
`1.0.219`; `Cargo.lock` pins it with a checksum; the crate is unpacked to
`~/.cargo/registry/src/<registry>/serde-1.0.219/`; it is compiled to
`target/debug/deps/libserde-<hash>.rmeta`. And then:

```
rustc --crate-name myapp src/main.rs \
      --extern serde=/…/target/debug/deps/libserde-9f3a1c.rmeta \
      --extern rand=/…/target/debug/deps/librand-2b77e0.rmeta
```

That is the entire interface. **`rustc` never looks in a directory for a crate
called `serde`.** Three consequences fall straight out of the shape:

- **The table is keyed per-importer**, so the name `serde` means one thing while
  compiling crate A and another while compiling crate B. That is what lets two
  major versions of one library live in one program — and Rust surfaces the
  clash at compile time (`expected serde::Value, found serde::Value`) rather
  than at run time.
- **The name in the source is not the package's name.** `json = { package =
  "serde_json" }` in the manifest makes the crate importable as `json`. The
  manifest owns the binding; the source only uses it. This is worth holding next
  to agenda item 1.6.
- **The build system knows the flags, and the compiler alone does not.** Cargo
  folds version, features and target into `-C metadata`, which appears in the
  artifact's *filename*. Enabling a feature changes the compiled interface, so
  it must change the identity — the exact requirement in agenda 3.4.

Zig is the same shape with the version layer removed entirely. `build.zig.zon`
names a dependency by a URL **and a content hash**, with no ranges and no
solver; `zig build` fetches it into `~/.cache/zig/p/<hash>/`; `build.zig` then
declares the graph in real code —
`exe.root_module.addImport("mylib", dep.module("mylib"))` — which becomes
`-M<name>=<path>` plus `--dep` edges on the command line. Content hash *is*
identity. There is no version solving to get wrong, and the price is that
deduplicating two copies of the same library is manual.

Go sits in this family with an unusual twist: **the version is in the path, and
sometimes in the specifier.** `go.mod` requires exact versions (never ranges),
and **Minimal Version Selection** picks the maximum of the minimums required
across the graph rather than the newest published — so adding a dependency can
never silently upgrade another one, and the build is reproducible with `go.sum`
carrying integrity only, not resolution. The artifact lands in
`$GOMODCACHE/github.com/x/y@v1.2.3/`, version in the directory name, and
`import "github.com/x/y/net/http"` maps onto it by longest module-path prefix.
The twist: **major version 2 and up must appear in the import path itself** —
`github.com/x/y/v2` is a *different* import path from `github.com/x/y`. Two
majors are two different packages by name, visible in the source, and the whole
duplicate-instance problem evaporates without any per-importer table.

### Family 2 — the build system resolves, then flattens to an ordered search path

Java, Python and C. The package manager does real work and then **throws the
structure away** at the handoff.

Maven resolves a `pom.xml` full of ranges and conflicts into exact coordinates,
downloads them to `~/.m2/repository/com/google/guava/guava/32.1.1-jre/…jar`, and
then invokes:

```
javac -cp ~/.m2/…/guava-32.1.1-jre.jar:~/.m2/…/other.jar …
```

An ordered list of jars. Every fact the resolver learned — which dependency
asked for what, who is a transitive dependency of whom — is gone. What survives
is first-match-wins over a flat namespace of `com/foo/Bar.class` entries. Hence:

- **Two jars containing the same class silently resolve to whichever is first.**
  This is "classpath hell", and it has no diagnostic.
- **One version of anything, ever.** Maven flattens conflicts by "nearest
  wins" (shortest path in the dependency tree — an arbitrary rule that surprises
  people constantly); Gradle picks the highest. Either way the graph collapses.
- The workaround, **shading**, is rewriting a library's package names to
  `com.myapp.shaded.guava` at build time. That is: faking a distinct identity by
  renaming, because the namespace offers no other way to have two.

Python is the same story with less machinery. pip resolves ranges, installs
**flat** into `site-packages/`, and the import system then searches `sys.path`
and never consults the installer's records. Two packages providing a top-level
`foo` both land in the same directory and the winner is path order. Virtualenvs
exist precisely *because* the flat namespace cannot hold two versions — the
isolation unit is the whole environment, not the dependency. (Newer tools —
`uv`, Poetry, PDM — add lockfiles and a content-addressed cache with hardlinks,
but the handoff to the interpreter is still `sys.path`.)

C and C++ are the limit case: there is no standard package manager, and every
attempt — pkg-config, CMake's `find_package`, Conan, vcpkg — ends by producing
**a set of `-I` and `-L` flags**. However good the resolver, the last mile is an
ordered ambient search, so all of C's failure modes return regardless. Conan and
vcpkg generate toolchain files pinning exact paths, which is them trying to be
Cargo through a keyhole. C++20 modules make the seam worse before better: the
build system must now also scan sources for `import` lines to compute the module
graph *before* compiling anything, which is why a standardized dependency-scan
format (P1689, `clang-scan-deps`) had to be invented.

### npm is the interesting hybrid, and its trouble is instructive

Node is ostensibly Family 2 — the runtime searches the filesystem — but **the
shape of the tree encodes the resolution**, which buys it something the
classpath cannot have.

`package.json` asks for `"lodash": "^4.17.0"`; npm resolves it against the
registry and writes the exact version plus a sha512 into `package-lock.json`;
and then it *materializes the resolved graph as directories*. Nesting is how a
version conflict is expressed: if two dependencies need incompatible versions,
one gets `node_modules/a/node_modules/lodash` and the upward walk from inside
`a` finds it first. **Per-importer scoping, implemented in the filesystem.**

The costs are all consequences of the layout being load-bearing:

- **Hoisting leaks phantom dependencies.** npm flattens what it can to the top
  level, so `require('x')` succeeds for a package you never declared, because it
  happened to be hoisted there by a transitive dependency. It works until that
  dependency drops it, in a release that is not yours.
- **Duplicate instances are invisible.** Two copies of one library are two
  module objects with two copies of its state, and `instanceof` across them
  fails. Nothing reports it.
- **Installing is building a directory tree**, which is slow and enormous.

The three responses map neatly onto the two families:

- **pnpm** keeps the tree but makes it honest: one content-addressable global
  store, hard links into `node_modules/.pnpm/<name>@<version>/`, and symlinks
  arranged so a package can only see what it declared. The filesystem now
  *enforces* the manifest instead of merely encoding it — phantom dependencies
  become errors.
- **Yarn PnP** deletes `node_modules` outright and generates `.pnp.cjs`: a
  literal table from (package, version) to a zip path, plus the dependency map
  of every package, with Node's resolver patched to consult it. This is Node
  moving to Family 1, in the same move Rust and Zig made from the start.
- **Deno** put the location in the specifier — the import *is* a URL — with a
  lockfile for integrity and a content-addressed cache.

### The generalization worth keeping

**How many versions of a library one program can contain equals how many
distinct names that library can have.**

Rust: a name per importing crate, so many. Go: the major version is part of the
name, so one per major. npm: a name per position in the tree, so many, with the
tree as the bookkeeping. Java, Python, C: one global name, therefore exactly
one version, and every workaround is a way to manufacture a second name —
shading, virtualenvs, symbol versioning in ELF.

The corollary for a cache key: in Family 1 a dependency's identity is
**known** — name, version, features, flags, hash — and can be hashed directly.
In Family 2 the identity is *whatever the search happened to find*, which is
why the search path itself has to enter the key, and why a `-I` reordering must
invalidate everything.

### One escape hatch every system needed

Developing a library and its consumer together defeats every registry: Cargo has
`path = "../foo"` and workspaces, npm has workspaces with symlinked local
packages, Go has `replace` directives, pip has `-e` editable installs, Zig has a
local `.path` dependency. It is always a way to say "this name resolves to that
directory on my disk, ignore the registry". Haard will want it early — the
compiler and its standard library are exactly this case.

### The build-system layer, in one table

| | requirement | resolution | store | handoff to the compiler |
|---|---|---|---|---|
| npm | range in `package.json` | `package-lock.json` + sha512 | `node_modules` tree, shape = resolution | upward directory walk |
| pnpm | range | lockfile | global CAS + hardlinks, symlink tree | walk, but fenced to declared deps |
| Yarn PnP | range | lockfile | zips | **`.pnp.cjs` table** |
| Cargo | range in `Cargo.toml` | `Cargo.lock` + checksum | `~/.cargo/registry` | **`--extern name=path`** |
| Go | **exact** version, MVS | `go.sum` (integrity only) | `$GOMODCACHE/mod@version` | exact dirs; version in path |
| Zig | **content hash**, no solver | `build.zig.zon` | `~/.cache/zig/p/<hash>` | **`-M name=path`** |
| Maven | range in `pom.xml` | nearest-wins, flattened | `~/.m2/repository` | ordered classpath |
| pip | range | flat install | `site-packages` | `sys.path` |
| CMake / Conan | varies | varies | varies | **`-I` / `-L` flags** |

---

## The comparison, in one table

| | names | mapped by | search | importer-relative? | binds or loads | cycles |
|---|---|---|---|---|---|---|
| C `#include` | path fragment | language | `-I` list, first wins | yes, for `""` | textual paste | n/a |
| C++20 modules | opaque name | build system | none specified | no | binds | forbidden |
| Java | type | language | classpath list, first wins | no | binds | allowed |
| Python | module/package | language | `sys.path` list, first wins | script dir only | **loads** | half-works |
| Node CJS | file | language | `node_modules` walk **upward** | **yes** | **loads** | half-works |
| Node ESM | URL | host (import map) | none / explicit map | yes (URL join) | loads, hoisted | diagnosed |
| Rust | item path | build system | `--extern` table | no | binds | forbidden between crates |
| Zig | module name or path | build system | `-M` table; paths fenced by root | yes, within a module | binds | mostly fine (lazy) |
| Go | **directory** | language + manifest | module cache, longest prefix | no | binds | **forbidden** |
| Haskell | module (flat per package) | build system | `-package-id` table | no | binds | forbidden across packages |
| C# | namespace | build system | `/reference` table | no | binds | allowed |
| OCaml | file basename | language | `-I` list, first wins | no | binds | forbidden |
| **Haard (0001/0006)** | **file / directory** | **language** | **open — item 1.9** | **open** | **open — 1.6** | **open — 1.3** |

## Item 1.9 in particular: one global table, or one table per root?

The question Haard actually faces, stated in its own terms. **A** is a single
`name -> path` map for the whole compilation. **B** is one such map per root, so
what `import zip.foo` resolves to depends on which root the importing file is
in.

| | what the compiler is handed | A or B | two versions of one library at once? | how a clash is broken |
|---|---|---|---|---|
| C / C++ | `-I` list, searched | **A**, and ambient | no | rename the header, or the include order |
| C++20 modules | nothing standardized; per-toolchain `name=path` flags | undefined | n/a in practice | — |
| Java | classpath list, searched | **A** | no | shading: rewrite the package names |
| Python | `sys.path` list, searched | **A** | no | a virtualenv per program |
| OCaml | `-I` list, searched | **A** | no | wrapped libraries, module aliases |
| Go | module cache, longest `go.mod` prefix | **A** | one per **major** | the major version is *in the name*: `x/y/v2` |
| Rust | `--extern name=path`, per crate | **B** | yes | distinct types; error at the use |
| Zig | `-M name=path` + `--dep`, per module | **B** | yes | content hash is the identity; dedup is manual |
| C# | `/reference:path.dll`, per project | **B** | mechanism yes, NuGet policy no | `extern alias` |
| Haskell | `-package-id name-ver-hash`, per package | **B** | mechanism yes, Cabal policy no | `PackageImports` |

Three things fall out of the table that are not obvious from any single row.

**The split is not old versus new, it is whether a package manager was designed
alongside the compiler.** Every A on the list predates its ecosystem's package
manager and had the search path already in place when one arrived; every B was
specified with the handoff in mind.

**Choosing B does not commit anyone to using it.** C# and Haskell are the proof:
both compilers take a per-importer table, and both ecosystems' build systems
unify to one version anyway. B is a mechanism, A is a policy, and a build system
can impose the policy on top of the mechanism at any time. The reverse is not
true — A cannot be given the mechanism later without changing what every import
means.

**A is survivable, and the escape hatch is always a second name.** Every A row's
last column is some way to manufacture a distinct name — shading, a virtualenv,
`/v2` in the path, a renamed header. Go's is the only cheap one, and it is cheap
precisely because it puts the second name *in the source*, where the resolver
never has to know about it.

---

## What this says about the open items

Not decisions — the shape of the evidence for each.

**1.3, cycles.** Everything that compiles a unit once and publishes a blob
forbids cycles across that unit: Go, Rust across crates, OCaml, Ada. Everything
that permits them either executes modules (Python, Node) and gets a half-built
object, or analyses lazily inside one unit (Zig, Rust within a crate). Since
the module/compilation design already commits to compile-then-serialize with
a `CHECKING` state as the cycle detector, the survey has no counterexample to
offer: rejecting is the option consistent with the model already chosen.

**1.6, what an import binds.** Java, Rust and Go all separate *loading* from
*naming*: the import brings the module into the compilation, and how its symbols
enter scope is a second, smaller question with a qualified-name fallback. Go's
extra move — the identifier used in code comes from the module itself, not from
its path — is what makes a deep path bearable without an alias on every line.

**1.7, collisions between star imports.** Java and Rust give the same answer:
ambiguity is an error **at the use**, not at the import. Java's version bites,
because adding a name to a library breaks a downstream file; Rust's is calmer
because a locally defined or explicitly imported name **shadows** a
glob-imported one, so only glob-versus-glob is ever ambiguous. If Haard reports
at the use, the shadowing rule is the part worth copying with it. Reporting at
the import is the option no surveyed language takes — worth knowing before
choosing it, since it means an unused, harmless collision is fatal.

**1.8, whether `*` recurses.** No language surveyed makes a wildcard recursive.
Java's on-demand import is one package deep; Python's `*` does not even reach
submodules; Go and Rust and Zig have no directory wildcard at all. Record 0006's
"only the direct children" reading is the unanimous one. On `import a.b.* as c`:
nothing here has an analogue, which is itself an argument for making it a
diagnostic rather than inventing a meaning.

**1.9, the search root.** Three findings, and they do not all point the same
way.

- *A list beats a single root* — a single root is what GOPATH was, and its
  failure is well documented. But Go's replacement was a **manifest**, not a
  longer list, and Rust, Zig and C#'s explicit tables exist for the same reason:
  an ordered list means first-match-wins, and first-match-wins means a local
  file can silently shadow a library. Every ambient-search language on the table
  has that bug (C's include order, Java's classpath, Python's `sys.path`), and
  every explicit-table language does not. If Haard takes the list — as
  the ModuleFinder discussion records Hadley deciding — then the list belongs in
  the cache key, and *reporting* a shadowed match is cheap insurance.
- *The importing file's directory should not take part* — Python's
  `sys.path[0]` and C's quoted include are the two systems that do it, and both
  are cited as design errors. Node does it wholesale and pays with duplicate
  module instances. Zig is the one that makes importer-relative resolution safe,
  and it does so by fencing it inside a module root, which Haard does not have,
  since a Haard module is a single file.
- *Union across roots is a real option, with a known price.* Python's PEP 420
  namespace packages are exactly the "union of `a/b` across every root" reading
  of the star-import question, and they do enable extending a package from
  outside. The price is that the package's contents depend on the entire search
  path, so the expansion hash of record 0006 would have to span roots and break
  ties — which is the argument already recorded against it.

**Where the seam falls, and what it costs to defer.** This is the question the
second half raises and the agenda does not yet have an item for. Record 0001's
environment-variable root puts Haard in Family 2: the compiler searches, and it
inherits shadowing, one version of anything, and a search path that must enter
the cache key. That is the right cost for a bootstrap — it is also C's, Java's,
Python's and OCaml's position, and it compiles a compiler perfectly well.

What is worth deciding early is only whether the **specifier leaves room**. The
exits other languages took from Family 2 are not equally cheap to retrofit:

- *Version in the import path*, Go's `/v2`. Retrofits for free, because it only
  makes names longer — the resolver never changes. Nothing in
  `foo.bar.baz` -> `foo/bar/baz.hd` forbids a segment that happens to name a
  version.
- *A per-importer name table*, Rust and Zig. The most capable answer, and the
  one that cannot be added quietly: it changes what a specifier means, so every
  import's resolution and every cache key moves with it. Node needed Yarn PnP
  and Java needed JPMS to attempt this retrofit, and neither replaced the old
  path — both ecosystems now carry two resolution systems at once.
- *Encoding the resolution in the directory tree*, npm's. The evidence against
  is unanimous.

Two smaller observations point at things Haard already plans. Zig's
`build.zig.zon` — **content hash as the whole identity, no ranges and no
solver** — costs nothing to adopt in a design that is already building a
content-addressed blob cache, and it deletes the version-solving layer outright;
its price is manual deduplication. And Go's MVS is the other minimal option:
exact versions in the manifest, maximum-of-the-minimums, reproducible with no
lockfile. Neither needs a SAT solver, and both are reachable later without
disturbing the resolver.

Finally, the escape hatch: every system on the table needed a way to say "this
name resolves to that directory on my disk, ignore everything else". Haard needs
it sooner than most, since the compiler and its standard library are exactly the
develop-together case.
