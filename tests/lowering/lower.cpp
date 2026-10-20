// Lowers a project to the IR and runs it in the interpreter (record 0077,
// stage 4).
//
//   lower <case directory>          the structured IR, then what running it
//                                   printed and how it ended
//   lower <case directory> --run    only what running it printed and how it
//                                   ended, written the way tests/emitter's
//                                   goldens end: '--- output:' when it
//                                   printed something, then '--- exit: N'
//
// The second is the oracle: every program of tests/emitter goes through it,
// and must end the way the C++ it compiled to did.
#include <haard/compilation/compilation.h>
#include <haard/ir/ir_interpreter.h>
#include <haard/ir/ir_printer.h>
#include <haard/lowering/ir_lowerer.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace haard;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: lower <case directory> [--run]\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    bool only_run = argc > 2 && std::strcmp(argv[2], "--run") == 0;
    std::filesystem::path table = directory / "table.tbl";
    Compilation compilation;

    if (std::filesystem::exists(table) && !compilation.set_roots(table)) {
        std::cout << "table error: " << compilation.get_error() << '\n';
        return 0;
    }

    std::ifstream entry_file(directory / "entry");
    std::string entry;

    std::getline(entry_file, entry);

    if (!compilation.build(directory / entry)) {
        std::cout << "did not check:\n";
        compilation.print_diagnostics(std::cout);
        return 0;
    }

    Target target;
    IrProgram program;
    IrLowerer lowerer;

    find_target("x86_64-linux", target);
    lowerer.set_compilation(&compilation);
    lowerer.set_target(&target);
    lowerer.set_program(&program);

    if (!lowerer.lower()) {
        std::cout << "--- not lowered: " << lowerer.get_error() << "\n";
        return 0;
    }

    if (!only_run) {
        IrPrinter printer;

        printer.set_program(&program);
        printer.print(std::cout);
        std::cout << "\n";
    }

    if (!lowerer.finish()) {
        std::cout << "--- not lowered: " << lowerer.get_error() << "\n";
        return 0;
    }

    IrObjectId main_function = {0};

    for (u32 i = 1; i < program.get_object_count(); i++) {
        if (program.get_object({i}).kind == IR_OBJECT_FUNCTION
            && program.get_object({i}).name == "main") {
            main_function.index = i;
        }
    }

    if (main_function.index == 0) {
        std::cout << "--- no main\n";
        return 0;
    }

    IrInterpreter interpreter;
    std::ostringstream output;

    interpreter.set_program(&program);
    interpreter.set_target(&target);
    interpreter.set_output(&output);

    IrRunResult ran = interpreter.run(main_function, {});
    std::string printed = output.str();

    // as the shell's $(...) keeps it: without its trailing newlines
    while (printed.size() > 0 && printed.back() == '\n') {
        printed.pop_back();
    }

    if (printed.size() > 0) {
        std::cout << "--- output:\n" << printed << "\n";
    }

    switch (ran.kind) {
    case IR_RUN_EXIT:
        std::cout << "--- exit: " << (ran.value & 0xff) << "\n";
        break;

    // C's abort(): what a shell reports for a process that took SIGABRT
    case IR_RUN_ABORT:
        std::cout << "--- exit: 134\n";
        break;

    default:
        std::cout << "--- stopped in @" << program.get_object(ran.function).name
                  << " at #" << ran.inst.index << ": " << ran.message << "\n";
        break;
    }

    return 0;
}
