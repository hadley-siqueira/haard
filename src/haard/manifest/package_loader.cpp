#include <haard/manifest/package_loader.h>
#include <sstream>

using namespace haard;

PackageLoader::PackageLoader() {
}

bool PackageLoader::load(ModuleFinder& finder,
                         const std::filesystem::path& file) {
    Manifest manifest;

    error.clear();
    packages.clear();
    finder.clear();

    if (!manifest.load(file)) {
        take(manifest);

        return false;
    }

    // the program's own root first, so the entry file falls under it
    u32 root = intern(finder, file);

    if (root == INVALID_ROOT) {
        finder.clear();

        return false;
    }

    // and only the **program's** prelude is asked. Record 0033 makes it a
    // property of the program: a library saying what every module of the
    // program should be given would be a library deciding for its importer
    for (const std::string& written : manifest.get_prelude()) {
        if (!finder.give_everyone(root, written)) {
            error = file.string() + ": the prelude asks for '" + written
                  + "', and nothing of that name is in view";

            finder.clear();

            return false;
        }
    }

    return true;
}

u32 PackageLoader::intern(ModuleFinder& finder,
                          const std::filesystem::path& file) {
    std::error_code failed;
    std::filesystem::path here = std::filesystem::weakly_canonical(file, failed);
    std::string key = (failed ? file : here).parent_path().string();

    auto found = packages.find(key);

    if (found != packages.end()) {
        return found->second;
    }

    Manifest manifest;

    if (!manifest.load(file)) {
        take(manifest);

        return INVALID_ROOT;
    }

    u32 root = finder.open_root(manifest.get_name(), manifest.get_directory());

    // written down **before** the dependencies are read, so a manifest that
    // reaches itself finds this root instead of reading the file again
    packages[key] = root;

    for (const Requirement& wanted : manifest.get_dependencies()) {
        if (wanted.path.size() == 0) {
            error = file.string() + ": '" + wanted.name
                  + "' is asked for by version, and there is nowhere for "
                    "versions to live yet -- write a path";

            return INVALID_ROOT;
        }

        // relative to the manifest that wrote it, never to the directory hdc
        // was run from: the same rule every path in the table follows, and
        // for the same reason -- the same command run from two places must
        // resolve to the same files
        u32 target = intern(finder, manifest.get_directory() / wanted.path
                                        / "haard.pkg");

        if (target == INVALID_ROOT) {
            return INVALID_ROOT;
        }

        // the name the importer writes, which is its own to choose: 'zip'
        // here may be a library that calls itself anything
        if (!finder.see(root, wanted.name, target)) {
            error = file.string() + ": '" + wanted.name
                  + "' is asked for twice";

            return INVALID_ROOT;
        }
    }

    return root;
}

// A manifest that did not read says why in its own logger, with a caret in
// the manifest -- it went through the same scanner and the same parser as any
// Haard file. What has no caret is a file that is not there, and that arrives
// as a message instead
void PackageLoader::take(Manifest& manifest) {
    std::ostringstream written;

    manifest.get_logger()->print(written);

    error = manifest.get_error().size() > 0 ? manifest.get_error()
                                            : written.str();
}

const std::string& PackageLoader::get_error() {
    return error;
}
