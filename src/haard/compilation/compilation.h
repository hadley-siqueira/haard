#ifndef HAARD_COMPILATION_H
#define HAARD_COMPILATION_H

#include <haard/ast_query/ast_query.h>
#include <haard/module_finder/module_finder.h>
#include <haard/module/module.h>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace haard {
    // Every module of one program, and the loop that finds them.
    //
    // The loop is the whole class: load a file, ask what it imports, resolve
    // each import against the root that file sits in, and put what comes back
    // at the end of the same list being walked. It ends when the walk catches
    // up with the list, which is when nothing new was reached.
    //
    // A module is interned by its resolved path, so a file imported by five
    // others is loaded once. That map is also what lets two modules import
    // each other without the loop spinning: the second time round the file is
    // already there and nothing is appended. Record 0007 allows those cycles,
    // so the loop must not merely survive them, it must not notice them.
    //
    // What it does NOT do yet, and it is deliberate: nothing here decides what
    // happens to an importer whose dependency failed (agenda 4.2), and nothing
    // orders the phases beyond scan-then-parse per file (agenda 4.1). Every
    // module is loaded and every diagnostic is reported; deciding to stop
    // early is a decision that has not been made.
    class Compilation {
        public:
            Compilation();
            ~Compilation();

        public:
            // Without a table the compilation is one file and its imports are
            // not followed, which is what 'hdc file.hd' has always done
            bool set_roots(const std::filesystem::path& table);
            const std::string& get_error();

            // loads the entry file and everything its imports reach. False
            // when any module logged an error, or when the entry file is
            // under no root of the table
            bool build(const std::filesystem::path& entry);

        public:
            u32 get_module_count();
            Module* get_module(u32 index);

            bool has_errors();

            // in module order, which is load order: the entry file first and
            // then whatever it reached, so the same program prints the same
            // diagnostics in the same sequence every time
            void print_diagnostics(std::ostream& out);

        private:
            // the index of the module for this path, loading nothing. New
            // paths are appended to the list the loop is walking
            u32 intern(const std::filesystem::path& path, u32 root);

            void load(u32 index);

            // resolves every import of this module and records what each one
            // reached on the module itself, in source order. Record 0013 made
            // that order load-bearing: an import binds nothing into the
            // importer's own tables, so a lookup walks this list and the
            // first module that answers is the one record 0009 means
            void resolve_imports(u32 index);

            void collect_symbols(u32 index);

            // a second walk, after the first has finished, because a type
            // written in the entry file may name a declaration of a module the
            // loop had not reached yet when it passed over it
            void collect_types();

            // a third walk, after the first has finished, because a use in
            // the entry file may name a symbol of a module the loop had not
            // reached yet when it passed over it
            void resolve_uses();

            void report(Module* module, u32 import, const std::string& message);

        private:
            ModuleFinder finder;
            AstQuery query;

            std::vector<Module*> modules;
            std::vector<std::filesystem::path> paths;

            // resolved path -> module index. Record 0010's cost 4 rests on
            // this being the key: two roots that the generator wrote as the
            // same directory are one module, for free
            std::map<std::string, u32> by_path;

            bool has_table;
            std::string error;
    };
}

#endif
