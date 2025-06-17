#ifndef HAARD_PRETTY_PRINTER_H
#define HAARD_PRETTY_PRINTER_H

#include <sstream>

#include <haard/ast/ast.h>
#include <haard/ast/import.h>
#include <haard/ast/module.h>
#include <haard/ast/variable.h>
#include <haard/ast/function.h>
#include <haard/ast/type.h>
#include <haard/ast/primitive_type.h>
#include <haard/ast/boxed_type.h>
#include <haard/ast/expression.h>
#include <haard/ast/literal.h>

namespace haard {
    class PrettyPrinter {
    public:
        PrettyPrinter();
        ~PrettyPrinter();

    public:
        void print(Ast* node, bool newline=false);

        void print_module(Module* node);
        void print_import(Import* node);
        void print_variable(Variable* node);

        void print_primitive_type(PrimitiveType* node);
        void print_boxed_type(BoxedType* node);

        void print_literal(Literal* node);

        void print(Token& token);

        std::string get_output();

    public:
        void emit(const std::string& msg);
        void print_newline();
        void print_indentation();
        void indent();
        void dedent();

    private:
        std::stringstream out;
        int indent_level;
    };
}

#endif
