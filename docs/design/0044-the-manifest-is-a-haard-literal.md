# 0044 — The manifest is a Haard literal

Status: **decided and built**, 2026-09-08. Hadley chose the shape after the
survey in
[notes/how-languages-write-a-manifest.md](notes/how-languages-write-a-manifest.md):
*"Let us go with B. The file is read by an entry point of its own and begins
with `{`."*

| | |
|---|---|
| The manifest is **data**, never a program | **decided** |
| It is written as a **Haard literal**, and the file opens with `{` | **decided**, Hadley |
| It is read by an **entry point of its own** — not as a module | **decided**, Hadley |
| Dependencies are a **list of records**, so two may share a name | **decided**, Hadley (form B) |
| A dependency says a **path** or a **version**, by name and never in a string | **decided** |
| `hdc` reads it and builds record 0010's table itself | **decided while writing it** |
| The **prelude** is the program's manifest and no other's | **decided while writing it** |

## What it looks like

```haard
# myapp/haard.pkg -- the only file written by hand.
{
    name: "myapp",
    version: "0.1",

    dependencies: [
        {name: "std", path: "../std"},
        {name: "shapes", path: "../shapes"}
    ],

    prelude: ["std.array", "std.string", "std.list", "std.hash", "std.file"]
}
```

`hdc --pkg myapp/haard.pkg --emit-cpp myapp/main.hd`.

## Why a Haard literal

Zig's answer, for Zig's reasons and one more.

**There is no parser to write.** The file goes through the scanner and the
parser this compiler already has, entered one rule lower: `Parser::parse_value`
reads an expression and then the end of the file. A manifest is a `{}` literal,
its dependencies are a `[]` literal, and record 0024's rule that a bracket
suspends the line rule is what lets it be written over many lines.

**And the diagnostics come with it.** A typo in a manifest is a rustc-shaped
error with a caret in the manifest, because it went through the same logger as
any Haard file:

```
error: 'prelide' is not something a manifest holds
 --> app/haard.pkg:6:5
  |
6 |     prelide: ["std.io"]
  |     ^^^^^^^
```

Nothing was written for that. A line-oriented format would have needed its own
parser, its own positions and its own error style — three things to keep in
step with the ones that already exist.

**The one more reason is the bootstrap.** The day `hpm` is written in Haard it
inherits the language's parser instead of growing a second one, which is the
best thing about Zig's answer and worth more here because rewriting this
compiler in Haard is the plan.

## Why a list of records, and not a record keyed by name

```haard
dependencies: [
    {name: "zip", path: "../zip2"},
    {name: "png", path: "../png"}
]
```

A record keyed by name — `dependencies: {zip: {...}}` — is shorter and is what
Cargo and ZON do. It also cannot say the one thing this whole design exists
for: **two entries may carry the same name**, which is one library at two
versions in one program. The table has always been able to express it (record
0010); the manifest above it now can too.

It costs a word per line. Hadley chose it knowing that.

## What `hdc` does with it

`PackageLoader` reads the manifest, follows each **path** dependency to that
directory's own `haard.pkg`, reads it too, and fills the `ModuleFinder` with
exactly what a table would have said: a root per manifest, each seeing its own
name (record 0010's `= name` line) and the name it gave each dependency.

So **record 0010 is untouched**. It decided what the compiler is handed; this
is the thing above it the record left for later, and nothing below the finder
can tell which of the two was read — the compilation suite has a case of each
and the goldens are the same shape.

Three things fall out of that shape:

- **A directory read twice is one root.** The manifests are interned by the
  directory they sit in, which is what makes a diamond one library and what
  makes two manifests that need each other stop rather than recurse.
- **A path is relative to the manifest that wrote it**, never to where `hdc`
  was run. Same rule as the table's paths, same reason: the same command from
  two directories must resolve to the same files.
- **The prelude is the program's alone.** A library declaring what every module
  of the program is given would be a library deciding for its importer.

`ModuleFinder` grew three calls for this — `open_root`, `see`,
`give_everyone` — and lost nothing: it still opens no file, parses nothing and
logs nothing. Reading a manifest is scanning and parsing, so it belongs to a
class that does those, not to the one that resolves paths.

## What is checked

- a key a manifest does not hold, **by name** — a typo in a dependency is the
  one thing a manifest must not swallow
- `name` missing
- a dependency with **neither** a path nor a version, or with **both**
- a dependency asked for **by version**: *there is nowhere for versions to live
  yet — write a path*, which is the marker for the stage after this one
- a name asked for twice by one manifest
- a prelude entry that reaches no file

## What is not written yet

- **versions**: where they live, how one is picked, and the lock file that
  comes with them. Record 0010 left this open on purpose and it stays open;
  `{name: "zip", version: "2.0"}` parses and is refused by name
- ~~**finding the manifest**~~ — **done the same day**: with neither `--pkg`
  nor `--roots` written, `hdc` takes the nearest `haard.pkg` **at or above the
  input file**, the way git finds `.git` and cargo finds `Cargo.toml`. So a
  program in three directories is built by naming one file:
  `hdc --emit-cpp myapp/main.hd`.

  It is the **Driver** that searches and nothing below it, which is the same
  discipline that keeps `getenv` there. Record 0010's rule is about
  **imports** — those never search — and this is one file found once, before
  any of that starts. The search follows the **file** and not the working
  directory, so the same command from two places finds the same manifest
- **`tool`**: the key is accepted and ignored, reserved for whatever else wants
  to keep its settings beside the manifest. It is pyproject's `[tool.*]`, and
  it costs one line now instead of a junk drawer later

## In code

`src/haard/manifest/manifest.{h,cpp}` reads one file;
`src/haard/manifest/package_loader.{h,cpp}` follows the paths and fills the
finder; `Parser::parse_value` is the entry point; `Compilation::set_package`
and `hdc --pkg` are the ways in, and `Driver::manifest_above` is the search
that makes the flag optional.

Cases: `tests/compilation/cases/a_program_read_from_manifests` (three
directories, three manifests, no table) and `a_manifest_that_does_not_read`
(the diagnostic, with its caret), and
`tests/programs/cases/a_program_built_from_manifests`, which builds and runs
through its own Makefile with `--pkg` and no table anywhere.
