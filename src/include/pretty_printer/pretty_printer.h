#ifndef HAARD_PRETTY_PRINTER_H
#define HAARD_PRETTY_PRINTER_H

#include <sstream>
#include "ast/ast.h"

namespace haard {
    class PrettyPrinter {
    public:
        PrettyPrinter();

    public:
        std::string get_output();

        void print_module(Module* module);
        void print_declaration(Declaration* decl);
        void print_import(Import* import);
        void print_class(Class* klass);
        void print_struct(Struct* st);
        void print_union(Union* u);
        void print_function(Function* function);
        void print_function_parameters(Function* function);

        void print_statement(Statement* stmt);
        void print_while_statement(WhileStatement* stmt);
        void print_for_statement(ForStatement* stmt);

        void print_if_statement(BranchStatement* stmt);
        void print_elif_statement(BranchStatement* stmt);
        void print_else_statement(BranchStatement* stmt);

        void print_return_statement(ReturnStatement* stmt);
        void print_compound_statement(CompoundStatement* stmt);
        void print_expression_statement(ExpressionStatement* stmt);

        void print_expression(Expression* expr);

        void print_cast_expression(Cast* expr);
        void print_not_in_expression(NotIn* expr);
        void print_index_expression(BinaryOperator* bin);
        void print_hash_pair_expression(HashPair* pair);

        void print_binary_operator(BinaryOperator* bin, bool no_space=false);

        void print_unary_operator(UnaryOperator* un, bool last=false);
        void print_logical_not_expression(UnaryOperator* un);
        void print_sizeof_expression(UnaryOperator* un);
        void print_new_expression(New* expr);
        void print_delete_expression(Delete* expr);
        void print_delete_array_expression(DeleteArray* expr);
        void print_parenthesis_expression(Parenthesis* expr);
        void print_call_expression(Call* expr);

        void print_type(Type* type);
        void print_subtyped_type(SubtypedType* type);
        void print_tuple_type(TupleType* tuple);
        void print_function_type(FunctionType* type);
        void print_named_type(NamedType* type);
        void print_array_type(ArrayType* type);
        void print_type_list(TypeList* tlist, const char* begin, const char* end);

        void print_identifier(Identifier* id);
        void print_generics(TypeList* g);

        void print_char_literal(CharLiteral* ch);
        void print_string_literal(StringLiteral* str);
        void print_tuple_expression(TupleLiteral* expr);
        void print_list_expression(ListLiteral* expr);
        void print_array_literal(ArrayLiteral* expr);
        void print_hash_literal(HashLiteral* expr);
        void print_sequence_expression(Sequence* expr);

        void print_expression_list(ExpressionList* list, const char* begin, const char* end, const char* sep=",");

    private:
        void indent();
        void dedent();
        void print_indentation();

    private:
        int indent_level;
        std::stringstream out;
    };
}
#endif
