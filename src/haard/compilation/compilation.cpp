#include <haard/compilation/compilation.h>
#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>
#include <haard/string_table/string_table.h>
#include <haard/name_resolver/use_resolver.h>
#include <haard/override_checker/override_checker.h>
#include <haard/statement_checker/statement_checker.h>
#include <haard/symbol_table/implicit_collector.h>
#include <haard/symbol_table/symbol_collector.h>
#include <haard/type_table/type_collector.h>
#include <stdexcept>

using namespace haard;

Compilation::Compilation() {
    has_table = false;
    parsing_only = false;
}

Compilation::~Compilation() {
    for (Module* module : modules) {
        delete module;
    }
}

bool Compilation::set_roots(const std::filesystem::path& table) {
    has_table = finder.load(table);

    if (!has_table) {
        error = finder.get_error();
    }

    return has_table;
}

const std::string& Compilation::get_error() {
    return error;
}

bool Compilation::build(const std::filesystem::path& entry) {
    u32 root = INVALID_ROOT;

    if (has_table) {
        root = finder.root_of_file(entry);

        if (root == INVALID_ROOT) {
            error = entry.string() + ": no root block of the table covers this "
                    "file";
            return false;
        }
    }

    intern(entry, root);

    // the list grows while it is walked, and that is the loop: everything an
    // import reached is appended and will be visited in its turn
    for (u32 i = 0; i < modules.size(); i++) {
        load(i);

        // asked for the tree and nothing else, so the loop appends nothing
        // and no phase runs. Resolving an import is already a question about
        // a program
        if (parsing_only) {
            continue;
        }

        // a module that did not parse has no tree to ask, and the phase gate
        // every reader of an Ast relies on says so. Its diagnostics are kept
        // and the other modules still load
        if (modules[i]->is_parsed()) {
            resolve_imports(i);
            collect_symbols(i);
        }
    }

    if (parsing_only) {
        return !has_errors();
    }

    collect_types();
    resolve_uses();
    check_statements();
    check_overrides();

    return !has_errors();
}

void Compilation::stop_after_parsing() {
    parsing_only = true;
}

u32 Compilation::get_module_count() {
    return (u32) modules.size();
}

Module* Compilation::get_module(u32 index) {
    return modules[index];
}

bool Compilation::has_errors() {
    if (error.size() > 0) {
        return true;
    }

    for (Module* module : modules) {
        if (module->get_logger()->has_errors()) {
            return true;
        }
    }

    return false;
}

void Compilation::print_diagnostics(std::ostream& out) {
    for (Module* module : modules) {
        module->get_logger()->print(out);
    }
}

u32 Compilation::intern(const std::filesystem::path& path, u32 root) {
    // The key is canonical and the path kept is the one that was passed. They
    // differ for the entry file, which arrives spelled the way the command
    // line spelled it: without this, an entry that some module also imports
    // would be interned twice under two spellings and compiled twice. The key
    // is an identity -- it is what agenda 3.4 will hash -- and the path is
    // only what gets opened
    std::string key = std::filesystem::weakly_canonical(path).string();
    auto found = by_path.find(key);

    if (found != by_path.end()) {
        return found->second;
    }

    Module* module = new Module();

    module->set_root(root);

    if (root != INVALID_ROOT) {
        module->set_name(finder.module_name_of_file(root, path));
    }

    modules.push_back(module);
    paths.push_back(path);
    by_path[key] = (u32) modules.size() - 1;

    return (u32) modules.size() - 1;
}

void Compilation::load(u32 index) {
    Scanner scanner;
    Parser parser;
    Module* module = modules[index];

    scanner.set_module(module);

    try {
        scanner.get_tokens(paths[index]);
    } catch (const std::exception& failure) {
        // SourceFile::open throws when the file is missing or unreadable.
        // The finder checked that every imported file is there, so this is
        // either the entry file or a file that went away in between; neither
        // has an offset to point at, so neither is a diagnostic
        error = failure.what();
        return;
    }

    if (module->get_logger()->has_errors()) {
        return;
    }

    parser.set_module(module);
    parser.parse();

    module->set_parsed(!module->get_logger()->has_errors());
}

void Compilation::resolve_imports(u32 index) {
    Module* module = modules[index];
    u32 root = module->get_root();

    if (root == INVALID_ROOT) {
        return;
    }

    query.set_module(module);

    for (u32 import : query.get_imports()) {
        std::string name = query.get_import_name(import);
        std::string alias = query.get_import_alias(import);
        bool star = query.is_star_import(import);

        // interned in the importer, because that is where the alias is
        // written and the only place it means anything. An import with no
        // 'as' gets no qualified form at all, not even by its last segment:
        // record 0008 makes the alias something that must be written to exist
        u32 interned = alias.size() > 0
                           ? module->get_strings()->intern(alias)
                           : INVALID_STRING;
        FindResult result = star ? finder.find_all(root, name)
                                 : finder.find(root, name);

        if (result.status == FIND_NO_ROOT) {
            // the table is what is wrong, not the source, so the message says
            // which entry is missing from which block
            report(module, import, "the root '" + finder.get_root_name(root) +
                   "' has no entry named '" +
                   name.substr(0, name.find('.')) + "'");
            continue;
        }

        if (result.status == FIND_NO_FILE) {
            // and here the source is what is wrong, so the message shows the
            // path: with two versions of a library in one program, the author
            // cannot tell which one he reached without being told
            report(module, import, "there is no " +
                   std::string(star ? "directory " : "file ") +
                   result.path.string());
            continue;
        }

        // the imported file belongs to its OWN root and not to this one, or
        // zip 1.0's internal imports would resolve inside whoever imported it
        //
        // What intern gives back is kept and not dropped: this loop runs over
        // the imports in the order the source wrote them, and a star's files
        // arrive sorted, so appending here is what builds the order record
        // 0009 resolves a bare name by. interning may append to 'modules',
        // which is a vector of pointers, so 'module' stays good
        if (star) {
            // one alias for the whole expansion (record 0006): it names the
            // pool, because there is no one module for it to name
            for (const std::filesystem::path& file : result.files) {
                module->add_dependency(intern(file, result.root), interned);
            }
        } else {
            module->add_dependency(intern(result.path, result.root), interned);
        }
    }
}

// Collecting is per module and depends on nothing but that module's tree, so
// it rides along in the same walk. That is the weakest possible answer to
// agenda 4.1, which has not decided a phase order, and it is deliberate: when
// 4.1 lands this call moves
void Compilation::collect_symbols(u32 index) {
    SymbolCollector collector;

    collector.set_module(modules[index]);
    collector.collect();
}

void Compilation::collect_types() {
    TypeCollector types;
    ImplicitCollector implicit;
    bool grew;

    types.set_compilation(this);
    implicit.set_compilation(this);

    // Round and round, and it is the module loop's shape a third time.
    // Record 0002 instantiates a generic by cloning its declaration into the
    // module that **wrote** it, which may be a module this loop has already
    // passed -- so one pass over the list is not enough. Each call takes only
    // the declarations it has not seen, so going round again is cheap and
    // reports nothing twice
    do {
        grew = false;

        for (u32 i = 0; i < modules.size(); i++) {
            if (modules[i]->is_parsed()) {
                grew = types.collect(i) || grew;
            }
        }

        // And only now the names an assignment declares by being written, in
        // the same round and never before it. Deciding that 'n = 1' declares n
        // means knowing that n is not already something -- and 'something'
        // includes a field of a **base**, which is a type and so is not there
        // until the pass above has run.
        //
        // It used to ride along in the walk that collects symbols, where the
        // answer cannot be known, and so it declared a local in front of every
        // inherited field: the field was never written and the local never
        // read, with nothing said. A declaration made here is typed by the
        // next round, which is what the loop was already for
        for (u32 i = 0; i < modules.size(); i++) {
            if (modules[i]->is_parsed()) {
                grew = implicit.declare(i) || grew;
            }
        }
    } while (grew);

    // and only then what a binding was given, because inferring it may resolve
    // a call whose candidate lives in a module the first loop reached later
    do {
        grew = false;

        for (u32 i = 0; i < modules.size(); i++) {
            if (modules[i]->is_parsed()) {
                grew = types.infer(i) || grew;
            }
        }
    } while (grew);
}

void Compilation::resolve_uses() {
    UseResolver uses;

    uses.set_compilation(this);

    for (u32 i = 0; i < modules.size(); i++) {
        if (modules[i]->is_parsed()) {
            uses.resolve(i);
        }
    }
}

void Compilation::check_statements() {
    StatementChecker statements;

    statements.set_compilation(this);

    for (u32 i = 0; i < modules.size(); i++) {
        if (modules[i]->is_parsed()) {
            statements.check(i);
        }
    }
}

void Compilation::check_overrides() {
    OverrideChecker overrides;

    overrides.set_compilation(this);

    for (u32 i = 0; i < modules.size(); i++) {
        if (modules[i]->is_parsed()) {
            overrides.check(i);
        }
    }
}

void Compilation::report(Module* module, u32 import,
                         const std::string& message) {
    module->get_logger()->error(query.get_import_offset(import),
                                query.get_import_length(import), message);
}
