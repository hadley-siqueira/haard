#ifndef HAARD_CPP_GENERATOR_H
#define HAARD_CPP_GENERATOR_H

#include <sstream>
#include <stack>

#include "ast/ast.h"
#include "ast/modules.h"

namespace haard {
    class CppGenerator {
    public:
        CppGenerator();

    public:
        std::string get_output();

        std::string build_cpp_header();
        void build_modules(Modules* modules);
        void build_module(Module* module);

        void build_module_classes(Module* module);

        void build_class(Class* klass);
        void build_member_variable(Variable* var);

        void build_function(Function* function);
        std::string build_function_header(Function* function);
        void build_function_body(Function* function);

        void build_statement(Statement* stmt);
        void build_while_statement(WhileStatement* stmt);
        void build_for_statement(ForStatement* stmt);
        void build_return_statement(ReturnStatement* stmt);
        void build_compound_statement(CompoundStatement* stmt);
        void build_expression_statement(ExpressionStatement* stmt);

        void build_expression(Expression* expr);

        void build_identifier(Identifier* id);

        void build_unary_operator(UnaryOperator* un, bool last=false);
        void build_binary_operator(BinaryOperator* bin, bool no_space=false);

        void build_index_expression(BinaryOperator* bin);
        void build_call_expression(Call* expr);
        void build_cast_expression(Cast* expr);

        void build_char_literal(CharLiteral* ch);
        void build_string_literal(StringLiteral* str);

        void build_expression_list(ExpressionList* list, const char* begin, const char* end, const char* sep=",");

        void build_type(Type* type);

    private:
        void set_output(std::stringstream* ss);
        void restore_output();

        void indent();
        void dedent();
        void print_indentation();

    private:
        int indent_level;
        std::stringstream* output;
        std::stringstream headers;
        std::stringstream classes;
        std::stringstream bodies;
        std::stack<std::stringstream*> output_stack;
    };
}

#endif
