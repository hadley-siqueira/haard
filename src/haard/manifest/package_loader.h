#ifndef HAARD_PACKAGE_LOADER_H
#define HAARD_PACKAGE_LOADER_H

#include <haard/manifest/manifest.h>
#include <haard/module_finder/module_finder.h>
#include <map>
#include <string>

namespace haard {
    // The roots of a program, read from the **manifests** instead of from a
    // table: the `haard.pkg` of the program and, through its path
    // dependencies, the manifest of every library it reaches.
    //
    // What comes out is what a table would have said — the same roots, the
    // same visible names, the same prelude — so nothing below the finder can
    // tell which of the two was read. Record 0010 is untouched by this: it
    // decided what the compiler is handed, and this is the thing above it
    // that the record left for later.
    //
    // It is a class of its own and not a second `load` on the finder, because
    // the finder opens no file, parses nothing and logs nothing — reading a
    // manifest is scanning and parsing, and a class that resolves paths has
    // no business doing either.
    class PackageLoader {
        public:
            PackageLoader();

        public:
            // Fills the finder from this manifest. False leaves the finder
            // empty and the reason in get_error() — a manifest's diagnostics
            // arrive there as text, with their caret, because the finder does
            // not log and neither does this
            bool load(ModuleFinder& finder, const std::filesystem::path& file);

            const std::string& get_error();

        private:
            // one manifest and everything it reaches, giving back the root it
            // became. INVALID_ROOT when it could not be read
            u32 intern(ModuleFinder& finder,
                       const std::filesystem::path& file);

            // a manifest that did not read: its diagnostics, or the message
            // for a file that is not there
            void take(Manifest& manifest);

        private:
            std::string error;

            // the manifests already read, by the directory they sit in, so a
            // library reached twice is one root — and so two manifests that
            // need each other stop
            std::map<std::string, u32> packages;
    };
}

#endif
