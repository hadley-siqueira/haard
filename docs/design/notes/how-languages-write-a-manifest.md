# How other languages write a manifest

Written 2026-09-08, before `haard.pkg` is designed. Companion to
[the-proposal-next-to-the-others.md](the-proposal-next-to-the-others.md), which
compared the **whole path** from what a user types to what the compiler is
handed; this one compares only the file at the top of that path — its
**syntax** — because that is the part a person edits every day.

Record [0010](../0010-a-table-per-root.md) closed the bottom half: `hdc` reads a
table of roots and searches nowhere. What is missing is the file above it, and
its equivalent in every other ecosystem is:

| ecosystem | manifest | lock | what the compiler is handed |
|---|---|---|---|
| Node / npm | `package.json` | `package-lock.json` | nothing — the runtime walks `node_modules` |
| Yarn PnP | `package.json` | `yarn.lock` | `.pnp.cjs`, a literal table |
| Rust | `Cargo.toml` | `Cargo.lock` | `--extern name=path`, per crate |
| Go | `go.mod` | `go.sum` | exact directories |
| Zig | `build.zig.zon` | none — the hash **is** the identity | `-M name=path` + `--dep` |
| C++ / Conan | `conanfile.txt` or `.py` | `conan.lock` | `-I` / `-L` flags |
| C++ / vcpkg | `vcpkg.json` | a registry baseline | flags |
| Java / Maven | `pom.xml` | resolver flattens | an ordered classpath |
| Python | `pyproject.toml` | `uv.lock` | `sys.path` |
| **Haard** | **`haard.pkg`** | — | **`roots.tbl`** |

## The one axis that decides everything else

**Is the manifest data, or is it a program?**

| data | a program |
|---|---|
| `package.json`, `Cargo.toml`, `go.mod`, `build.zig.zon`, `pom.xml`, `vcpkg.json`, `pyproject.toml` | `conanfile.py`, Bazel's `BUILD` (Starlark), Gradle's Kotlin DSL, Zig's own `build.zig` |

A manifest that is a program can express anything — conditional dependencies,
platform switches, generated targets. It also **cannot be read without being
run**, which means no tool can safely rewrite it, no cache key can be computed
from it without executing arbitrary code, and "what does this project depend
on" is answerable only by running the build. Every ecosystem that started there
has since grown a data file beside it (Gradle has version catalogs, Bazel grew
`MODULE.bazel`, Zig grew `build.zig.zon`).

Haard's answer is settled by its own constraint: **`hdc` will read this file**,
and a compiler that runs arbitrary code to find out what to compile is not a
compiler anybody can trust. Data.

## The five syntaxes, side by side

### `package.json` — JSON

```json
{
  "name": "myapp",
  "version": "0.1.0",
  "dependencies": {
    "zip": "^2.0.0",
    "mylib": "file:../mylib"
  },
  "devDependencies": { "jest": "^29" },
  "scripts": { "build": "tsc" }
}
```

**For:** every language on earth parses JSON; there is exactly one way to write
any value; tooling can rewrite it safely.

**Against:** **no comments**, which is the single most complained-about thing in
the format's history — a dependency cannot carry the sentence saying why it is
pinned. Everything is a string, so meaning is smuggled into **string
microformats**: `"^2.0.0"`, `"file:../mylib"`, `"git+https://…#semver:^1"`.
Each is a little language with its own parser and its own surprises. And the
file is a junk drawer: manifest, scripts and every tool's configuration in one
object.

### `Cargo.toml` — TOML

```toml
[package]
name = "myapp"
version = "0.1.0"
edition = "2021"

[dependencies]
zip = "2.0"                                        # the short form
png = { version = "1.0", features = ["encode"] }   # the long one
mylib = { path = "../mylib" }
other = { git = "https://example.com/o", tag = "v1.2" }

[workspace]
members = ["app", "libs/*"]
```

**For:** comments; flat, named sections that read like prose; **a short form and
a long form of the same thing**, so the common case is one line and the rare one
is explicit; **named keys instead of microformats** — `path =`, `git =`,
`version =` say what they are; workspaces for several libraries developed
together.

**Against:** TOML has corners that bite — inline tables versus `[dependencies.png]`
sections are the same thing written two ways, `[[bin]]` arrays-of-tables are
their own puzzle, and dotted keys mean `a.b = 1` and `[a] b = 1` are equal.
And `Cargo.toml` has grown into a language of its own: features, profiles,
`[patch]`, `[replace]`, targets. That is not TOML's fault, but the format
invited it.

### `go.mod` — a bespoke line-oriented grammar

```
module example.com/myapp

go 1.22

require (
    example.com/zip v2.0.0
    example.com/png v1.0.3 // indirect
)

replace example.com/zip => ../zip
exclude example.com/bad v1.0.0
```

**For:** the **smallest grammar of the five** — a handful of verbs, one per
line, no nesting at all. Comments. It is designed to be edited by a machine
(`go mod edit`) as easily as by a person, and it shows. Versions are **exact**:
Go picks with MVS and needs no solver, so there is no range syntax to learn and
no `^` versus `~` folklore.

**Against:** bespoke, so nothing else parses it; deliberately not extensible,
so no tool may store anything in it; and the module path doubles as a URL,
which bakes an identity decision into the syntax.

### `build.zig.zon` — the language's own literal

```zig
.{
    .name = "myapp",
    .version = "0.1.0",
    .dependencies = .{
        .zip = .{
            .url = "https://example.com/zip-2.0.tar.gz",
            .hash = "1220a1b2c3…",
        },
        .mylib = .{ .path = "../mylib" },
    },
    .paths = .{ "src", "build.zig" },
}
```

**For:** it is Zig's own value syntax, so the compiler already has the parser
and a reader already knows the shape — **one grammar for the language and its
manifest**. Comments. Structured values, no microformats. And the **hash is the
identity**: no registry, no version solver, no lock file, and a dependency that
changed is a different dependency by construction.

**Against:** the dot-prefixed syntax is unfamiliar even to Zig readers at first;
a hash is 60 characters that a human cannot check or remember (though
`zig fetch --save` writes it, so nobody types one); and with no versions there
is no upgrade story in the file itself — updating means fetching a new hash.

### `pom.xml` — XML

```xml
<project>
  <groupId>com.example</groupId>
  <artifactId>myapp</artifactId>
  <version>0.1.0</version>
  <dependencies>
    <dependency>
      <groupId>com.example</groupId>
      <artifactId>zip</artifactId>
      <version>2.0</version>
    </dependency>
  </dependencies>
</project>
```

**For:** nothing is implicit; a schema validates it; parent POMs and BOMs keep
versions aligned across dozens of modules, which is a real problem at that
scale and which nobody else solved as thoroughly.

**Against:** **five lines per dependency**; the inheritance that makes BOMs
possible also means the effective configuration is not in the file you are
reading — `mvn help:effective-pom` exists because of it.

### The rest, in one line each

**`vcpkg.json`** is JSON again, and says almost nothing: versions come from a
registry baseline, not from the file. **`conanfile.txt`** is an INI so small it
is charming, and `conanfile.py` is the program version of the same thing with
all the costs above. **`pyproject.toml`** is worth one idea on its own:
`[project]` is standardised and `[tool.<name>]` is **reserved for third
parties** — the junk-drawer problem solved by namespacing rather than by
discipline.

## What is worth taking into `haard.pkg`

| take | from | why |
|---|---|---|
| **data, never a program** | everyone but Bazel/Conan.py | `hdc` reads it; a compiler that runs code to find out what to compile is not trustworthy |
| **comments** | everyone but JSON | a pin needs the sentence saying why |
| **a line-oriented grammar with named blocks** | `go.mod`, and `roots.tbl` itself | the smallest grammar that works, no new parser to write, and it already matches the file this repo generates |
| **short form and long form** | Cargo | `zip = 2.0` for the common case, `zip = path ../zip` when it is not |
| **named keys, never microformats** | Cargo, Zig | `path`, `git`, `version` — not `"file:../x"` |
| **exact versions, no ranges** | Go | no solver, reproducible, and it keeps versions out of the compiler where record 0010 wants them |
| **a path dependency as a first-class form** | Cargo `path`, Zig `.path`, go.mod `replace` | it is what developing two libraries together needs, and it is what Hadley asked for first |
| **a reserved namespace for other tools** | pyproject's `[tool.*]` | costs one paragraph now and prevents the junk drawer later |
| **the prelude block** | nobody — it is Haard's own | record 0033 puts it in the table; it belongs in the file a human writes |

And what to leave out, each because someone else paid for it:

- **scripts** (npm) — a manifest that runs things is a build system with a
  disguise
- **features and options** (Cargo, Conan) — real problems, but they multiply
  the resolver and there is nothing yet to resolve
- **inheritance** (Maven) — the effective file stops being the file
- **ranges** (`^`, `~`, `>=`) — they require a solver, and the solver is the
  part every ecosystem regrets
- **a lock file, for now** — with no versions there is nothing to lock. The day
  versions arrive it arrives with them

## The strawman

```
# myapp/haard.pkg -- the only file written by hand.
name = myapp
version = 0.1

[dependencies]
std = path ../std
zip = path ../libs/zip
png = 1.0                # later: a version, from wherever versions live

[prelude]
import std.array
import std.string
import std.list
import std.hash

[tool.editor]            # reserved: hdc reads the name and ignores the block
```

One grammar, and it is the one this repository already parses: `#` to the end of
the line, `[block]` headers, `key = value` inside them, one per line, no
nesting. `ModuleFinder::load` reads a file shaped exactly like this today.

**The thing it is not:** a program, a registry, a solver, or a build system.
It says what this library is and what it needs, and something above `hdc` turns
that into the table `hdc` already reads.

## The one open question the syntax cannot decide

Whether the file, one day, becomes a **Haard literal** the way ZON is a Zig
literal:

```haard
{name: "myapp", version: "0.1", dependencies: {std: {path: "../std"}}}
```

That is agenda 1.23's brace literal, and the day it exists this file could
become one — one grammar for the language and its manifest, which is the best
thing about Zig's answer. Nothing about the strawman above blocks it: a
line-oriented file can be replaced by a literal without changing what it means.
