#ifndef HAARD_MANIFEST_H
#define HAARD_MANIFEST_H

#include <haard/module/module.h>
#include <filesystem>
#include <string>
#include <vector>

namespace haard {
    // One thing a library needs, by name and by where it comes from. Exactly
    // one of 'path' and 'version' is written, which is Cargo's shape and not
    // npm's: what a dependency is, is said with a **named key** and never
    // smuggled into a string -- no "file:../x", no "^2.0.0"
    struct Requirement {
        std::string name;
        std::string path;
        std::string version;

        // where in the manifest it was written, so a diagnostic about it has
        // somewhere to point
        u32 node;
    };

    // What a library says about itself: `haard.pkg`, the only file a person
    // writes by hand.
    //
    // It is **a Haard literal**, and the file opens with '{'. Hadley,
    // 2026-09-08, choosing form B of the survey in
    // notes/how-languages-write-a-manifest.md:
    //
    //     {
    //         name: "myapp",
    //         version: "0.1",
    //
    //         dependencies: [
    //             {name: "std", path: "../std"},
    //             {name: "zip", path: "../libs/zip"}
    //         ],
    //
    //         prelude: ["std.array", "std.string"]
    //     }
    //
    // Which is the answer Zig's `build.zig.zon` gives and for the same two
    // reasons: the parser is the language's own, so there is none to write and
    // none to keep in step -- and the day `hpm` is written in Haard it
    // inherits that parser instead of growing a second one.
    //
    // **It is data and never a program.** A manifest that has to be run to be
    // read cannot be rewritten by a tool, cannot be a cache key, and answers
    // 'what does this depend on' only by executing. Every ecosystem that
    // started there has since grown a data file beside it.
    //
    // What it is NOT: a registry, a solver, a lock file or a build system. It
    // says what this library is and what it needs; something above `hdc`
    // turns that into the table of roots `hdc` already reads (record 0010).
    class Manifest {
        public:
            Manifest();

        public:
            // Reads one manifest. False leaves the reason in the module's
            // logger -- with a caret in the manifest, since it went through
            // the same scanner and the same parser as any Haard file -- and
            // in get_error() when there was no file to point into
            bool load(const std::filesystem::path& file);

            const std::string& get_error();
            Logger* get_logger();

            // the directory the manifest sits in, which is the root a
            // dependency's relative path is resolved against
            const std::filesystem::path& get_directory();

            const std::string& get_name();
            const std::string& get_version();
            const std::vector<Requirement>& get_dependencies();

            // the imports every module of the program is given, record 0033.
            // Only the manifest of the program being compiled is asked: a
            // prelude is a property of the program and not of a library
            const std::vector<std::string>& get_prelude();

        private:
            // the top level literal, key by key
            void read(u32 node);

            // 'dependencies: [{name: "std", path: "../std"}]'
            void read_dependencies(u32 list);
            void read_requirement(u32 record);

            // 'prelude: ["std.array"]'
            void read_prelude(u32 list);

            // the text of a string literal, without its quotes and with no
            // escape taken apart: a path and a module name hold neither
            std::string text_of(u32 node);
            std::string key_of(u32 pair);

            void report(u32 node, const std::string& message);

            AstNodeKind kind_of(u32 node);
            u32 first_child(u32 node);
            u32 sibling_of(u32 node);

        private:
            Module module;
            std::string error;
            std::filesystem::path directory;

            std::string name;
            std::string version;
            std::vector<Requirement> dependencies;
            std::vector<std::string> prelude;
    };
}

#endif
