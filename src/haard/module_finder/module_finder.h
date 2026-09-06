#ifndef HAARD_MODULE_FINDER_H
#define HAARD_MODULE_FINDER_H

#include <haard/haard.h>
#include <filesystem>
#include <string>
#include <vector>

namespace haard {
    // no root: what root_of_file gives back for a file no block covers, and
    // what an unknown name resolves to inside a block
    const u32 INVALID_ROOT = 0xffffffff;

    enum FindStatus {
        FIND_OK,

        // the import's first segment names nothing the importing root can
        // see. This is an error in the table, and the message can say which
        // entry is missing from which block
        FIND_NO_ROOT,

        // the entry is there and nothing is at the end of it: no such .hd for
        // a plain import, no such directory for a star. This is an error in
        // the source, and the message has to show the path, or the author
        // cannot tell which version of the library he reached
        FIND_NO_FILE
    };

    // One shape for both questions, so that a caller can pick between them
    // with a ternary on is_star_import and switch on the status once
    struct FindResult {
        FindStatus status;

        // the file, for a plain import; the directory that was listed, for a
        // star. Empty when the status is FIND_NO_ROOT
        std::filesystem::path path;

        // the star expansion, sorted. Always empty for a plain import
        std::vector<std::filesystem::path> files;

        // the root the found file belongs to, which is the target of the
        // import's first segment and NOT the importing root: 'import
        // zip.compress' written in app reaches a file whose own imports are
        // zip's. INVALID_ROOT unless the status is FIND_OK
        u32 root;
    };

    // One import the table gives to every module of the program, resolved.
    // Record 0033: the prelude is a **list**, read once and never a module,
    // so what a caller gets is a file and the root it belongs to -- the same
    // pair a FindResult carries, because it is interned the same way
    struct PreludeImport {
        std::filesystem::path path;
        u32 root;
    };

    // one block of the table: a root, and what it can see
    struct Root {
        std::string name;
        std::filesystem::path path;

        // The '= name' of the header line is in here too, pointing at this
        // very block. That is what record 0010 means by turning the self-name
        // rule into a line of the block: the resolver below has no case for
        // a library importing itself, because there is nothing special left
        std::vector<std::string> visible_names;
        std::vector<u32> visible_roots;
    };

    // Turns an import into a file path, and nothing else. It does not open
    // the file, does not scan it and does not know what a token is.
    //
    // Two disciplines it keeps on purpose:
    //
    // It does not log. An unresolved import is a diagnostic pointing at the
    // import node, and this class has never seen one; the caller holds the
    // node index and writes the message from a FindResult. A malformed table
    // is not a diagnostic at all -- there is no source position to point at
    // -- so load reports it the way Driver::read_arguments reports a bad
    // command line: false, and a message.
    //
    // It does not read the environment. getenv stays in the Driver, which
    // already owns argv, so a test builds a fixture tree and never touches
    // the process it runs in.
    class ModuleFinder {
        public:
            ModuleFinder();

        public:
            // Reads the table written by record 0010. False leaves the finder
            // empty and the reason in get_error()
            bool load(const std::filesystem::path& table);
            const std::string& get_error();

        public:
            // which root a file on the command line belongs to, by longest
            // prefix. INVALID_ROOT when no block covers it
            u32 root_of_file(const std::filesystem::path& file);

            // 'a.b.c', written inside root, to a file
            FindResult find(u32 root, const std::string& name);

            // 'a.b.*', written inside root, to every .hd directly in the
            // directory a/b, sorted. Not recursive, per record 0006
            FindResult find_all(u32 root, const std::string& name);

            // the dotted name of a file inside a root: app/main.hd under the
            // root ../app is the module app.main
            std::string module_name_of_file(u32 root,
                                            const std::filesystem::path& file);

            // What every module of this program imports without writing it,
            // in the order the table wrote them, and empty when there is no
            // 'prelude' block. Record 0033.
            //
            // Resolved when the table was read and not per importing root,
            // which is the whole reason this is a vector and not a question:
            // 'target_of' answers an import against the visibility list of
            // the root that wrote it, so resolving these per module would
            // fail in every root whose block does not name the library --
            // once per module, blaming an import nobody wrote
            const std::vector<PreludeImport>& get_prelude();

        public:
            u32 get_root_count();
            const std::string& get_root_name(u32 root);
            const std::filesystem::path& get_root_path(u32 root);

        private:
            // a dependency line kept aside for the second pass, with the text
            // the user wrote so that the message can quote it back
            struct Dependency {
                u32 block;
                std::string name;
                std::string text;
                std::filesystem::path path;
                u32 line;
            };

            // a prelude entry kept aside for the second pass, with the
            // line so that a bad one is reported where it was written
            struct PendingImport {
                std::string name;
                u32 line;
            };

            bool read_header(const std::string& text, u32 line,
                             const std::filesystem::path& directory);
            bool read_dependency(const std::string& text, u32 line,
                                 const std::filesystem::path& directory,
                                 std::vector<Dependency>& pending);

            // 'prelude' at column zero, and 'import <module>' under it
            bool read_prelude_header(u32 line);
            bool read_prelude_import(const std::string& text, u32 line,
                                     std::vector<PendingImport>& pending);
            bool resolve_prelude(const std::vector<PendingImport>& pending,
                                 const std::filesystem::path& directory);

            bool add_visible(u32 block, const std::string& name, u32 target,
                             u32 line);
            bool fail(u32 line, const std::string& message);

            u32 visible_root(u32 root, const std::string& name);
            u32 block_with_path(const std::filesystem::path& path);

            // the target root of an import's first segment, INVALID_ROOT when
            // this root cannot see it
            u32 target_of(u32 root, const std::vector<std::string>& segments);

            std::vector<std::string> split_name(const std::string& name);
            std::filesystem::path normalize(const std::filesystem::path& path);

            // component by component, never on the string: '/sys/zip/1.0' is
            // a string prefix of '/sys/zip/1.0-beta/x.hd' and is not its root
            bool starts_with(const std::filesystem::path& file,
                             const std::filesystem::path& root);

        private:
            std::vector<Root> roots;

            // the table's path as it was given, for the messages. The
            // canonical one would put this machine's home in every error
            std::string table_name;
            std::string error;

            u32 block;

            // The 'prelude' block, and INVALID_ROOT when the table has none.
            // It is a Root with an **empty path**: nothing lives in it, and
            // what it holds is a visibility list and a list of imports. That
            // is what lets its entries resolve through 'find' unchanged --
            // and block names are not unique (two versions of one library are
            // two blocks both named 'zip'), so resolving them globally by
            // name was never available
            u32 prelude_block;
            std::vector<PreludeImport> prelude;
    };
}

#endif
