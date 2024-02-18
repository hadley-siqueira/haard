#include <cassert>
#include "pretty_printer/pretty_printer.h"

using namespace haard;

PrettyPrinter::PrettyPrinter() {
    indent_level = 0;
}

std::string PrettyPrinter::get_output() {
    return out.str();
}

void PrettyPrinter::print_module(Module* module) {
    /*for (int i = 0; i < module->imports_count(); ++i) {
        print_import(module->get_import(i));
        out << '\n';
    }*/

    for (int i = 0; i < module->declarations_count(); ++i) {
        print_declaration(module->get_declaration(i));
    }
}

void PrettyPrinter::print_declaration(Declaration* decl) {
    int kind = decl->get_kind();

    switch (kind) {
    case AST_IMPORT:
        print_import((Import*) decl);
        break;

    case AST_FUNCTION:
        print_function((Function*) decl);
        break;

    case AST_CLASS:
        print_class((Class*) decl);
        break;

    case AST_STRUCT:
        print_struct((Struct*) decl);
        break;
    }

    out << '\n';
}

void PrettyPrinter::print_import(Import* import) {
    int i;

    out << "import ";

    for (i = 0; i < import->path_count() - 1; ++i) {
        out << import->get_path_token(i).get_value() << ".";
    }

    out << import->get_path_token(i).get_value();

    if (import->has_alias()) {
        out << " as " << import->get_alias().get_value();
    }
}

void PrettyPrinter::print_class(Class* klass) {
    out << "class ";
    out << klass->get_name().get_value();

    if (klass->get_generics()) {
        print_generics(klass->get_generics());
    }

    if (klass->get_super_type()) {
        out << "(";
        print_type(klass->get_super_type());
        out << ")";
    }

    out << ":\n";
    indent();

    for (int i = 0; i < klass->variables_count(); ++i) {
        Variable* var = klass->get_variable(i);

        print_indentation();
        out << var->get_name().get_value() << " : ";
        print_type(var->get_type());
        out << '\n';
    }

    for (int i = 0; i < klass->functions_count(); ++i) {
        print_indentation();
        print_function(klass->get_function(i));
        out << '\n';
    }

    dedent();
}

void PrettyPrinter::print_struct(Struct* st) {
    out << "struct ";
    out << st->get_name().get_value();

    if (st->get_generics()) {
        print_generics(st->get_generics());
    }

    if (st->get_super_type()) {
        out << "(";
        print_type(st->get_super_type());
        out << ")";
    }

    out << ":\n";
    indent();

    for (int i = 0; i < st->variables_count(); ++i) {
        Variable* var = st->get_variable(i);

        print_indentation();
        out << var->get_name().get_value() << " : ";
        print_type(var->get_type());
        out << '\n';
    }

    for (int i = 0; i < st->functions_count(); ++i) {
        print_indentation();
        print_function(st->get_function(i));
        out << '\n';
    }

    dedent();
}

void PrettyPrinter::print_function(Function* function) {
    out << "def ";
    out << function->get_name().get_value();

    print_generics(function->get_generics());

    out << " : ";
    print_type(function->get_return_type());
    out << '\n';
    indent();

    print_function_parameters(function);
    print_statement(function->get_statements());

    dedent();
}

void PrettyPrinter::print_function_parameters(Function* function) {
    Variable* param;

    for (int i = 0; i < function->parameters_count(); ++i) {
        param = function->get_parameter(i);
        print_indentation();
        out << "@" << param->get_name().get_value() << " : ";
        print_type(param->get_type());
        out << "\n";
    }
}

void PrettyPrinter::print_statement(Statement* stmt) {
    switch (stmt->get_kind()) {
    case AST_COMPOUND_STATEMENT:
        print_compound_statement((CompoundStatement*) stmt);
        break;

    case AST_EXPRESSION_STATEMENT:
        print_expression_statement((ExpressionStatement*) stmt);
        break;

    case STMT_WHILE:
        print_while_statement((WhileStatement*) stmt);
        break;

    case STMT_FOR:
        print_for_statement((ForStatement*) stmt);
        break;

    case STMT_RETURN:
        print_return_statement((ReturnStatement*) stmt);
        break;

    default:
        assert(false && "invalid statement");
        break;
    }
}

void PrettyPrinter::print_while_statement(WhileStatement* stmt) {
    print_indentation();
    out << "while ";
    print_expression(stmt->get_condition());
    out << ":\n";
    indent();
    print_compound_statement(stmt->get_statements());
    dedent();
    out << '\n';
}

void PrettyPrinter::print_for_statement(ForStatement* stmt) {
    print_indentation();
    out << "for ";

    if (stmt->is_foreach()) {
        print_expression(stmt->get_range());
    } else {
        print_expression_list(stmt->get_init(), "", "");
        out << "; ";
        print_expression(stmt->get_test());
        out << "; ";
        print_expression_list(stmt->get_update(), "", "");
    }

    out << ":\n";
    indent();
    print_compound_statement(stmt->get_statements());
    dedent();
    out << '\n';
}

void PrettyPrinter::print_return_statement(ReturnStatement* stmt) {
    print_indentation();
    out << "return";

    if (stmt->get_expression()) {
        out << ' ';
        print_expression(stmt->get_expression());
    }
}

void PrettyPrinter::print_compound_statement(CompoundStatement* stmt) {
    if (stmt->statements_count() == 0) {
        out << "pass";
        return;
    }

    for (int i = 0; i < stmt->statements_count(); ++i) {
        print_statement(stmt->get_statement(i));
    }
}

void PrettyPrinter::print_expression_statement(ExpressionStatement* stmt) {
    print_indentation();
    print_expression(stmt->get_expression());
    out << '\n';
}

void PrettyPrinter::print_expression(Expression* expr) {
    BinaryOperator* bin = (BinaryOperator*) expr;
    UnaryOperator* un = (UnaryOperator*) expr;
    Literal* literal = (Literal*) expr;

    if (expr == nullptr) {
        return;
    }

    switch (expr->get_kind()) {
    case AST_ID:
        print_identifier((Identifier*) expr);
        break;

    // Binary operators
    case EXPR_ASSIGNMENT:
    case EXPR_BITWISE_AND_ASSIGNMENT:
    case EXPR_BITWISE_XOR_ASSIGNMENT:
    case EXPR_BITWISE_OR_ASSIGNMENT:
    case EXPR_BITWISE_NOT_ASSIGNMENT:
    case EXPR_DIVISION_ASSIGNMENT:
    case EXPR_INTEGER_DIVISION_ASSIGNMENT:
    case EXPR_MINUS_ASSIGNMENT:
    case EXPR_MODULO_ASSIGNMENT:
    case EXPR_PLUS_ASSIGNMENT:
    case EXPR_TIMES_ASSIGNMENT:
    case EXPR_SHIFT_LEFT_LOGICAL_ASSIGNMENT:
    case EXPR_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT:
    case EXPR_SHIFT_RIGHT_LOGICAL_ASSIGNMENT:
    case EXPR_LOGICAL_OR:
    case EXPR_LOGICAL_AND:
    case EXPR_EQUAL:
    case EXPR_NOT_EQUAL:
    case EXPR_LESS_THAN:
    case EXPR_LESS_THAN_OR_EQUAL:
    case EXPR_GREATER_THAN:
    case EXPR_GREATER_THAN_OR_EQUAL:
    case EXPR_IN:
    case EXPR_INCLUSIVE_RANGE:
    case EXPR_EXCLUSIVE_RANGE:
    case EXPR_PLUS:
    case EXPR_MINUS:
    case EXPR_TIMES:
    case EXPR_DIVISION:
    case EXPR_MODULO:
    case EXPR_INTEGER_DIVISION:
    case EXPR_POWER:
    case EXPR_BITWISE_OR:
    case EXPR_BITWISE_XOR:
    case EXPR_BITWISE_AND:
    case EXPR_SHIFT_LEFT_LOGICAL:
    case EXPR_SHIFT_RIGHT_ARITHMETIC:
    case EXPR_SHIFT_RIGHT_LOGICAL:
        print_binary_operator(bin);
        break;

    case EXPR_DOT:
    case EXPR_ARROW:
        print_binary_operator(bin, true);
        break;

    case EXPR_CAST:
        print_cast_expression((Cast*) expr);
        break;

    case EXPR_NOT_IN:
        print_not_in_expression((NotIn*) expr);
        break;

    case EXPR_INDEX:
        print_index_expression(bin);
        break;

    case EXPR_CALL:
        print_call_expression((Call*) expr);
        break;

    case EXPR_HASH_PAIR:
        print_hash_pair_expression((HashPair*) expr);
        break;

    case EXPR_LOGICAL_NOT:
        print_logical_not_expression(un);
        break;

    case EXPR_SIZEOF:
        print_sizeof_expression(un);
        break;

    case EXPR_NEW:
        print_new_expression((New*) expr);
        break;

    case EXPR_DELETE:
        print_delete_expression((Delete*) expr);
        break;

    case EXPR_DELETE_ARRAY:
        print_delete_array_expression((DeleteArray*) expr);
        break;

    case EXPR_PARENTHESIS:
        print_parenthesis_expression((Parenthesis*) expr);
        break;

    case EXPR_UNARY_PLUS:
    case EXPR_UNARY_MINUS:
    case EXPR_ADDRESS_OF:
    case EXPR_DEREFERENCE:
    case EXPR_BITWISE_NOT:
    case EXPR_PRE_INCREMENT:
    case EXPR_PRE_DECREMENT:
        print_unary_operator(un);
        break;

    case EXPR_POS_INCREMENT:
    case EXPR_POS_DECREMENT:
        print_unary_operator(un, true);
        break;

    case EXPR_LITERAL_INTEGER:
    case EXPR_LITERAL_FLOAT:
    case EXPR_LITERAL_DOUBLE:
        out << literal->get_token().get_value();
        break;

    case EXPR_LITERAL_CHAR:
        print_char_literal((CharLiteral*) expr);
        break;

    case EXPR_LITERAL_STRING:
        print_string_literal((StringLiteral*) expr);
        break;

    case EXPR_LITERAL_TUPLE:
        print_tuple_expression((TupleLiteral*) expr);
        break;

    case EXPR_LITERAL_LIST:
        print_list_expression((ListLiteral*) expr);
        break;

    case EXPR_LITERAL_ARRAY:
        print_array_literal((ArrayLiteral*) expr);
        break;

    case EXPR_LITERAL_HASH:
        print_hash_literal((HashLiteral*) expr);
        break;

    case EXPR_SEQUENCE:
        print_sequence_expression((Sequence*) expr);
        break;
    }
}

void PrettyPrinter::print_cast_expression(Cast* expr) {
    print_expression(expr->get_expression());
    out << " as ";
    print_type(expr->get_type());
}

void PrettyPrinter::print_not_in_expression(NotIn* expr) {
    print_expression(expr->get_left());
    out << " not in ";
    print_expression(expr->get_right());
}

void PrettyPrinter::print_index_expression(BinaryOperator* bin) {
    print_expression(bin->get_left());
    out << '[';

    print_expression(bin->get_right());
    out << ']';
}

void PrettyPrinter::print_hash_pair_expression(HashPair* pair) {
    print_expression(pair->get_left());
    out << ": ";
    print_expression(pair->get_right());
}

void PrettyPrinter::print_logical_not_expression(UnaryOperator* un) {
    if (un->get_token().get_kind() == TK_LOGICAL_NOT) {
        print_unary_operator(un);
    } else {
        out << "not ";
        print_expression(un->get_expression());
    }
}

void PrettyPrinter::print_sizeof_expression(UnaryOperator* un) {
    out << "sizeof(";
    print_expression(un->get_expression());
    out << ")";
}

void PrettyPrinter::print_new_expression(New* expr) {
    out << "new ";
    print_type(expr->get_type());
    print_expression_list(expr->get_arguments(), "(", ")");
}

void PrettyPrinter::print_delete_expression(Delete* expr) {
    out << "delete ";
    print_expression(expr->get_expression());
}

void PrettyPrinter::print_delete_array_expression(DeleteArray* expr) {
    out << "delete[] ";
    print_expression(expr->get_expression());
}

void PrettyPrinter::print_parenthesis_expression(Parenthesis* expr) {
    out << "(";
    print_expression(expr->get_expression());
    out << ")";
}

void PrettyPrinter::print_call_expression(Call* expr) {
    print_expression(expr->get_object());

    if (expr->get_arguments()) {
        print_expression_list(expr->get_arguments(), "(", ")");
    } else {
        out << "()";
    }
}

void PrettyPrinter::print_binary_operator(BinaryOperator* bin, bool no_space) {
    out << "(";
    print_expression(bin->get_left());

    const char* oper = bin->get_token().get_value();

    if (no_space) {
        out << oper;
    } else {
        out << ' ' << oper << ' ';
    }

    print_expression(bin->get_right());
    out << ")";
}

void PrettyPrinter::print_unary_operator(UnaryOperator* un, bool last) {
    out << "(";

    const char* oper = un->get_token().get_value();

    if (last) {
        print_expression(un->get_expression());
        out << oper;
    } else {
        out << oper;
        print_expression(un->get_expression());
    }

    out << ")";
}

void PrettyPrinter::print_type(Type* type) {
    if (type == nullptr) return;

    int kind = type->get_kind();

    switch (kind) {
    case TYPE_I8:
        out << "i8";
        break;

    case TYPE_U8:
        out << "u8";
        break;

    case TYPE_I16:
        out << "i16";
        break;

    case TYPE_U16:
        out << "u16";
        break;

    case TYPE_I32:
        out << "i32";
        break;

    case TYPE_U32:
        out << "u32";
        break;

    case TYPE_I64:
        out << "i64";
        break;

    case TYPE_U64:
        out << "u64";
        break;

    case TYPE_F32:
        out << "f32";
        break;

    case TYPE_F64:
        out << "f64";
        break;

    case TYPE_CHAR:
        out << "char";
        break;

    case TYPE_UCHAR:
        out << "uchar";
        break;

    case TYPE_SHORT:
        out << "short";
        break;

    case TYPE_USHORT:
        out << "ushort";
        break;

    case TYPE_INT:
        out << "int";
        break;

    case TYPE_UINT:
        out << "uint";
        break;

    case TYPE_LONG:
        out << "long";
        break;

    case TYPE_ULONG:
        out << "ulong";
        break;

    case TYPE_FLOAT:
        out << "float";
        break;

    case TYPE_DOUBLE:
        out << "double";
        break;

    case TYPE_VOID:
        out << "void";
        break;

    case TYPE_STR:
        out << "str";
        break;

    case TYPE_SYMBOL:
        out << "sym";
        break;

    case TYPE_BOOL:
        out << "bool";
        break;

    case TYPE_POINTER:
    case TYPE_REFERENCE:
    case TYPE_LIST:
        print_subtyped_type((SubtypedType*) type);
        break;

    case TYPE_ARRAY:
        print_array_type((ArrayType*) type);
        break;

    case TYPE_TUPLE:
        print_tuple_type((TupleType*) type);
        break;

    case TYPE_FUNCTION:
        print_function_type((FunctionType*) type);
        break;

    case TYPE_NAMED:
        print_named_type((NamedType*) type);
        break;
    }
}

void PrettyPrinter::print_subtyped_type(SubtypedType* type) {
    int kind = type->get_kind();

    switch (kind) {
    case TYPE_POINTER:
        print_type(type->get_subtype());
        out << '*';
        break;

    case TYPE_REFERENCE:
        print_type(type->get_subtype());
        out << '&';
        break;

    case TYPE_LIST:
        out << '[';
        print_type(type->get_subtype());
        out << ']';
        break;
    }
}

void PrettyPrinter::print_tuple_type(TupleType* tuple) {
    print_type_list(tuple->get_types(), "(", ")");
}

void PrettyPrinter::print_function_type(FunctionType* type) {
    print_type_list(type->get_param_types(), "(", ")");
    out << " -> ";
    print_type(type->get_return_type());
}

void PrettyPrinter::print_named_type(NamedType* type) {
    print_identifier(type->get_identifier());
}

void PrettyPrinter::print_array_type(ArrayType* type) {
    print_type(type->get_subtype());
    out << "[";

    if (type->get_expression()) {
        print_expression(type->get_expression());
    }

    out << "]";
}

void PrettyPrinter::print_type_list(TypeList* tlist, const char* begin, const char* end) {
    if (tlist == nullptr) {
        return;
    }

    int i;

    out << begin;

    for (i = 0; i < tlist->types_count() - 1; ++i) {
        print_type(tlist->get_type(i));
        out << ", ";
    }

    print_type(tlist->get_type(i));
    out << end;
}

void PrettyPrinter::print_identifier(Identifier* id) {
    if (id->has_global_alias()) {
        out << "::";
    } else if (id->has_alias()) {
        out << id->get_alias().get_value();
        out << "::";
    }

    out << id->get_name().get_value();
    print_generics(id->get_generics());
}

void PrettyPrinter::print_generics(TypeList* g) {
    print_type_list(g, "<", ">");
}

void PrettyPrinter::print_char_literal(CharLiteral* ch) {
    out << '\'' << ch->get_token().get_value() << '\'';
}

void PrettyPrinter::print_string_literal(StringLiteral* str) {
    char c = '"';

    if (str->get_token().get_kind() == TK_LITERAL_SINGLE_QUOTE_STRING) {
        c = '\'';
    }

    out << c << str->get_token().get_value() << c;
}

void PrettyPrinter::print_tuple_expression(TupleLiteral* expr) {
    print_expression_list(expr->get_expressions(), "(", ")");
}

void PrettyPrinter::print_list_expression(ListLiteral* expr) {
    print_expression_list(expr->get_expressions(), "[", "]");
}

void PrettyPrinter::print_array_literal(ArrayLiteral* expr) {
    print_expression_list(expr->get_expressions(), "{", "}");
}

void PrettyPrinter::print_hash_literal(HashLiteral* expr) {
    print_expression_list(expr->get_expressions(), "{", "}");
}

void PrettyPrinter::print_sequence_expression(Sequence* expr) {
    print_expression_list(expr->get_expressions(), "(", ")", ";");
}

void PrettyPrinter::print_expression_list(ExpressionList* list, const char* begin, const char* end, const char* sep) {
    if (list == nullptr) {
        return;
    }

    int i;
    out << begin;

    if (list->expressions_count() > 0) {
        for (i = 0; i < list->expressions_count() - 1; ++i) {
            print_expression(list->get_expression(i));
            out << sep << " ";
        }

        print_expression(list->get_expression(i));
    }

    out << end;
}

void PrettyPrinter::indent() {
    ++indent_level;
}

void PrettyPrinter::dedent() {
    --indent_level;
}

void PrettyPrinter::print_indentation() {
    for (int i = 0; i < indent_level; ++i) {
        out << "    ";
    }
}
