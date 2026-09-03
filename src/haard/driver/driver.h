#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <haard/compilation/compilation.h>
#include <haard/emitter/emitter.h>
#include <haard/pretty_printer/pretty_printer.h>
#include <string>

namespace haard {
    // Runs the compilation phases in order and decides what happens between
    // them. A phase that logged an error stops the ones after it, so nothing
    // downstream ever works on a file the phase before rejected
    class Driver {
        public:
            Driver();

        public:
            // the process exit code: 0 on success, 1 when the file did not
            // compile, 2 when the command line itself is wrong
            int run(int argc, char* argv[]);

        private:
            bool read_arguments(int argc, char* argv[]);
            void print_usage(std::ostream& out);

            int compile();

        private:
            Compilation compilation;
            PrettyPrinter pretty_printer;
            Emitter emitter;

            std::string path;
            std::string roots;
            std::string program;
            bool show_help;
            bool show_tokens;
            bool show_pretty_print;
            bool emit_cpp;
    };
}

#endif
