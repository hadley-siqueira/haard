#ifndef HAARD_PRETTY_PRINTER_H
#define HAARD_PRETTY_PRINTER_H

#include <sstream>
#include <haard/ast/ast.h>

namespace haard {
    class PrettyPrinter {
        public:
            void print(Ast* node);

        public:
            void print_module(Ast* node);
            void print_module_imports(Ast* node);

            void print_import(Ast* node);
            void print_import_path(Ast* node);
            void print_import_alias(Ast* node);

            void print_identifier(Ast* node);

            void print(Token& token);
            void print(const std::string& s);
            void print_separated_by(Ast* node, const std::string& s);

        private:
            std::stringstream out;

    };
}

#endif
