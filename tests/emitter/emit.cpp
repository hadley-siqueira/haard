// Runs the whole front end over one project and writes the C++ the Emitter
// produced, so that the golden is the generated program:
//
//   emit <case directory>
//
// The directory holds 'table.tbl' (or none), 'entry' naming the file to start
// from, and the project's sources. What comes out on stdout is either the C++
// or, when the emitter refused, the reason it gave -- and a refusal is a
// golden like any other, because saying "this cannot be emitted yet" is the
// behaviour and writing C++ that means something else is the bug.
//
// The suite's real check is not this text. run.sh compiles what comes out and
// runs it, and puts the exit status at the end of the golden: a transpiler is
// right when its output runs and gives the right answer, and everything else
// is a proxy for that.
#include <haard/compilation/compilation.h>
#include <haard/emitter/emitter.h>
#include <fstream>
#include <iostream>

using namespace haard;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: emit <case directory>\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    std::filesystem::path table = directory / "table.tbl";
    Compilation compilation;
    Emitter emitter;

    if (std::filesystem::exists(table) && !compilation.set_roots(table)) {
        std::cout << "table error: " << compilation.get_error() << '\n';
        return 0;
    }

    std::ifstream entry_file(directory / "entry");
    std::string entry;

    std::getline(entry_file, entry);

    // the front end has to be clean first: the emitter reads the tables the
    // type phase filled, and emitting from a program that did not check would
    // be reading answers nobody worked out
    if (!compilation.build(directory / entry)) {
        std::cout << "did not check:\n";
        compilation.print_diagnostics(std::cout);

        return 0;
    }

    emitter.set_compilation(&compilation);

    if (!emitter.emit(std::cout)) {
        std::cout << "cannot emit: " << emitter.get_error() << '\n';
    }

    return 0;
}
