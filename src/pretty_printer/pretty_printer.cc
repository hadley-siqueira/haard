#include <cassert>
#include <iostream>
#include "pretty_printer/pretty_printer.h"

using namespace haard;

PrettyPrinter::PrettyPrinter() {
    indent_level = 0;
}

std::string PrettyPrinter::get_output() {
    return out.str();
}

void PrettyPrinter::print(Ast* node) {
    if (node == nullptr) {
        return;
    }

    switch (node->get_kind()) {
    case AST_UNKNOWN:
        break;

    case AST_MODULE:
        print_module(node);
        break;

    /* Import */
    case AST_IMPORT:
        print_import(node);
        break;

    case AST_IMPORT_PATH:
        print_import_path(node);
        break;

    case AST_IMPORT_ALIAS:
        print_import_alias(node);
        break;

    case AST_IMPORT_PATH_MEMBER:
        print_import_path_member(node);
        break;

    /* Definitions */
    case AST_FUNCTION:
        print_function(node);
        break;

    case AST_PARAMETERS:
        print_parameters(node);
        break;

    case AST_PARAMETER:
        print_parameter(node);
        break;

    /* Statements */
    case AST_COMPOUND_STATEMENT:
        print_statements(node);
        break;

    case AST_WHILE:
        print_while(node);
        break;

    case AST_RETURN:
        print_return(node);
        break;

    /* Expressions */
    case AST_EXPRESSION:
        print_expression(node);
        break;

    case AST_EXPRESSION_WITH_SEMICOLON:
        print_expression(node, true);
        break;

    case AST_TIMES:
        print_times(node);
        break;

    case AST_DIVISION:
        print_division(node);
        break;

    case AST_MODULO:
        print_modulo(node);
        break;

    case AST_INTEGER_DIVISION:
        print_integer_division(node);
        break;

    case AST_POWER:
        print_power(node);
        break;

    case AST_BITWISE_OR:
        print_bitwise_or(node);
        break;

    case AST_BITWISE_XOR:
        print_bitwise_xor(node);
        break;

    case AST_BITWISE_AND:
        print_bitwise_and(node);
        break;

    case AST_SHIFT_LEFT_LOGICAL:
        print_shift_left_logical(node);
        break;

    case AST_SHIFT_RIGHT_LOGICAL:
        print_shift_right_logical(node);
        break;

    case AST_SHIFT_RIGHT_ARITHMETIC:
        print_shift_right_arithmetic(node);
        break;

    case AST_LOGICAL_NOT:
        print_logical_not(node);
        break;

    case AST_NOT:
        print_not(node);
        break;

    case AST_ADDRESS_OF:
        print_address_of(node);
        break;

    case AST_DEREFERENCE:
        print_dereference(node);
        break;

    case AST_BITWISE_NOT:
        print_bitwise_not(node);
        break;

    case AST_UNARY_MINUS:
        print_unary_minus(node);
        break;

    case AST_UNARY_PLUS:
        print_unary_plus(node);
        break;

    case AST_PRE_INCREMENT:
        print_pre_increment(node);
        break;

    case AST_PRE_DECREMENT:
        print_pre_decrement(node);
        break;

    case AST_SIZEOF:
        print_sizeof(node);
        break;

    case AST_DOT:
        print_dot(node);
        break;

    case AST_ARROW:
        print_arrow(node);
        break;

    case AST_INDEX:
        print_index(node);
        break;

    case AST_CALL:
        print_call(node);
        break;

    case AST_ARGUMENTS:
        print_arguments(node);
        break;

    case AST_NAMED_ARGUMENT:
        print_argument_name(node);
        break;

    case AST_POS_DECREMENT:
        print_pos_decrement(node);
        break;

    case AST_POS_INCREMENT:
        print_pos_increment(node);
        break;

    case AST_PARENTHESIS:
        print_parenthesis(node);
        break;

    case AST_TUPLE:
        print_tuple(node);
        break;

    case AST_SEQUENCE:
        print_sequence(node);
        break;

    case AST_LIST:
        print_list(node);
        break;

    case AST_ARRAY:
        print_array(node);
        break;

    case AST_HASH:
        print_hash(node);
        break;

    case AST_HASH_PAIR:
        print_hash_pair(node);
        break;

    case AST_GENERIC_APPLICATION:
        print_generic_application(node);
        break;

    case AST_SCOPE:
        print_scope(node);
        break;

    case AST_ID:
        print_identifier(node);
        break;

    /* Literals */
    case AST_THIS:
        out << "this";
        break;

    case AST_NULL:
        out << "null";
        break;

    case AST_LITERAL_BOOLEAN:
        out << node->get_value();
        break;

    case AST_LITERAL_CHAR:
        out << "'" << node->get_value() << "'";
        break;

    case AST_LITERAL_INTEGER:
        out << node->get_value();
        break;

    case AST_LITERAL_FLOAT:
        out << node->get_value();
        break;

    case AST_LITERAL_DOUBLE:
        out << node->get_value();
        break;

    case AST_LITERAL_STRING:
        out << "\"" << node->get_value() << "\"";
        break;

    /* Types */
    case AST_TYPE_I8:
        out << "i8";
        break;

    case AST_TYPE_U8:
        out << "u8";
        break;

    case AST_TYPE_I16:
        out << "i16";
        break;

    case AST_TYPE_U16:
        out << "u16";
        break;

    case AST_TYPE_I32:
        out << "i32";
        break;

    case AST_TYPE_U32:
        out << "u32";
        break;

    case AST_TYPE_I64:
        out << "i64";
        break;

    case AST_TYPE_U64:
        out << "u64";
        break;

    case AST_TYPE_F32:
        out << "f32";
        break;

    case AST_TYPE_F64:
        out << "f64";
        break;

    case AST_TYPE_CHAR:
        out << "char";
        break;

    case AST_TYPE_UCHAR:
        out << "uchar";
        break;

    case AST_TYPE_SHORT:
        out << "short";
        break;

    case AST_TYPE_USHORT:
        out << "ushort";
        break;

    case AST_TYPE_INT:
        out << "int";
        break;

    case AST_TYPE_UINT:
        out << "uint";
        break;

    case AST_TYPE_LONG:
        out << "long";
        break;

    case AST_TYPE_ULONG:
        out << "ulong";
        break;

    case AST_TYPE_FLOAT:
        out << "float";
        break;

    case AST_TYPE_DOUBLE:
        out << "double";
        break;

    case AST_TYPE_VOID:
        out << "void";
        break;

    case AST_TYPE_STR:
        out << "str";
        break;

    case AST_TYPE_SYMBOL:
        out << "sym";
        break;

    case AST_TYPE_BOOL:
        out << "bool";
        break;

    case AST_TYPE_POINTER:
        print(node->get_child(0));
        out << '*';
        break;

    case AST_TYPE_REFERENCE:
        print(node->get_child(0));
        out << '&';
        break;

    case AST_TYPE_LIST:
        print_list_type(node);
        break;

    case AST_TYPE_ARRAY:
        print_array_type(node);
        break;

    case AST_TYPE_TUPLE:
        print_tuple_type(node);
        break;

    case AST_TYPE_FUNCTION:
        print_function_type(node);
        break;

    case AST_TYPE_NAMED:
        print_named_type(node);
        break;

    /* Others */
    case AST_GENERICS:
        print_generics(node);
        break;

    default:
        std::cout << "unhandled case: " << node->get_kind() << "\n";
        break;
    }
}

void PrettyPrinter::print_module(Ast* module) {
    AstKind kind;

    for (auto i = 0; i < module->children_count(); ++i) {
        kind = module->get_child(i)->get_kind();

        print(module->get_child(i));
        out << "\n";
    }
}

void PrettyPrinter::print_import(Ast* import) {
    size_t i;

    out << "import ";
    print(import->get_child(0));
    print(import->get_child(1));
}

void PrettyPrinter::print_import_path(Ast* path) {
    size_t i;

    for (i = 0; i < path->children_count() - 1; ++i) {
        print(path->get_child(i));
        out << ".";
    }

    print(path->get_child(i));
}

void PrettyPrinter::print_import_path_member(Ast* member) {
    out << member->get_value();
}

void PrettyPrinter::print_import_alias(Ast* alias) {
    out << " as ";
    out << alias->get_value();
}

void PrettyPrinter::print_list_type(Ast* node) {
    out << '[';
    print(node->get_child(0));
    out << ']';
}

void PrettyPrinter::print_times(Ast* node) {
    print_binop(node, "*");
}

void PrettyPrinter::print_division(Ast* node) {
    print_binop(node, "/");
}

void PrettyPrinter::print_modulo(Ast* node) {
    print_binop(node, "%");
}

void PrettyPrinter::print_integer_division(Ast* node) {
    print_binop(node, "//");
}

void PrettyPrinter::print_power(Ast* node) {
    print_binop(node, "**");
}

void PrettyPrinter::print_bitwise_or(Ast* node) {
    print_binop(node, "|");
}

void PrettyPrinter::print_bitwise_xor(Ast* node) {
    print_binop(node, "^");
}

void PrettyPrinter::print_bitwise_and(Ast* node) {
    print_binop(node, "&");
}

void PrettyPrinter::print_shift_left_logical(Ast* node) {
    print_binop(node, "<<");
}

void PrettyPrinter::print_shift_right_logical(Ast* node) {
print_binop(node, ">>>");
}

void PrettyPrinter::print_shift_right_arithmetic(Ast* node) {
print_binop(node, ">>");
}

void PrettyPrinter::print_dot(Ast* node) {
    print(node->get_child(0));
    out << ".";
    print(node->get_child(1));
}

void PrettyPrinter::print_arrow(Ast* node) {
    print(node->get_child(0));
    out << "->";
    print(node->get_child(1));
}

void PrettyPrinter::print_parenthesis(Ast* node) {
    out << "(";
    print(node->get_child(0));
    out << ")";
}

void PrettyPrinter::print_generic_application(Ast* node) {
    print(node->get_child(0));
    print(node->get_child(1));
}

void PrettyPrinter::print_scope(Ast* scope) {
    if (scope->children_count() == 1) {
        out << "::";
        print(scope->get_child(0));
    } else {
        print(scope->get_child(0));
        out << "::";
        print(scope->get_child(1));
    }
}

void PrettyPrinter::print_identifier(Ast* id) {
    out << id->get_value();
}

/*
void PrettyPrinter::print_class(Class* klass) {
    out << "class ";
    out << klass->get_name().get_value();

    if (klass->get_generics()) {
       // print_generics(klass->get_generics());
    }

    if (klass->get_super_type()) {
        out << "(";
        //print_type(get_super_type());
        out << ")";
    }

    out << ":\n";
    indent();

    for (int i = 0; i < klass->variables_count(); ++i) {
        Variable* var = klass->get_variable(i);

        print_indentation();
        out << var->get_name().get_value() << " : ";
        //print_type(var->get_type());
        out << '\n';
    }

    for (int i = 0; i < klass->functions_count(); ++i) {
        print_indentation();
        //print_function(klass->get_function(i));
        out << '\n';
    }

    dedent();
}*/

/*
void PrettyPrinter::print_struct(Struct* st) {
    out << "struct ";
    out << st->get_name().get_value();

    if (st->get_generics()) {
       // print_generics(st->get_generics());
    }

    if (st->get_super_type()) {
        out << "(";
        //print_type(st->get_super_type());
        out << ")";
    }

    out << ":\n";
    indent();

    for (int i = 0; i < st->variables_count(); ++i) {
        Variable* var = st->get_variable(i);

        print_indentation();
        out << var->get_name().get_value() << " : ";
        //print_type(var->get_type());
        out << '\n';
    }

    for (int i = 0; i < st->functions_count(); ++i) {
        print_indentation();
        //print_function(st->get_function(i));
        out << '\n';
    }

    dedent();
}*/

/*
void PrettyPrinter::print_union(Union* u) {
    out << "union ";
    out << u->get_name().get_value();

    if (u->get_generics()) {
       // print_generics(u->get_generics());
    }

    if (u->get_super_type()) {
        out << "(";
        //print_type(u->get_super_type());
        out << ")";
    }

    out << ":\n";
    indent();

    for (int i = 0; i < u->variables_count(); ++i) {
        Variable* var = u->get_variable(i);

        print_indentation();
        out << var->get_name().get_value() << " : ";
        //print_type(var->get_type());
        out << '\n';
    }

    for (int i = 0; i < u->functions_count(); ++i) {
        print_indentation();
        //print_function(u->get_function(i));
        out << '\n';
    }

    dedent();
}*/

void PrettyPrinter::print_function(Ast* function) {
    out << "def ";
    out << function->get_value();

    print_generics(function->get_child(AST_GENERICS));

    out << " : ";
    print(function->get_child(AST_RETURN_TYPE)->get_child(0));
    out << '\n';
    indent();

    print(function->get_child(AST_PARAMETERS));
    print(function->get_child(AST_COMPOUND_STATEMENT));

    dedent();
}

void PrettyPrinter::print_parameters(Ast* parameters) {
    Ast* param;

    for (int i = 0; i < parameters->children_count(); ++i) {
        print_indentation();
        print(parameters->get_child(i));
        out << "\n";
    }

    if (parameters->children_count() > 0) {
        out << "\n";
    }
}

void PrettyPrinter::print_parameter(Ast* parameter) {
    out << "@" << parameter->get_value() << " : ";
    print(parameter->get_child(0));

    if (parameter->get_child(1)) {
        out << " = ";
        print(parameter->get_child(1));
    }
}

void PrettyPrinter::print_while(Ast* stmt) {
    print_indentation();
    out << "while ";
    print(stmt->get_child(0));
    out << ":\n";
    indent();
    print(stmt->get_child(1));
    dedent();
    out << '\n';
}

/*
void PrettyPrinter::print_for_statement(ForStatement* stmt) {
    print_indentation();
    out << "for ";

    if (stmt->is_foreach()) {
        print_expression(stmt->get_range());
    } else {
        //print_expression_list(stmt->get_init(), "", "");
        out << "; ";
        print_expression(stmt->get_test());
        out << "; ";
        //print_expression_list(stmt->get_update(), "", "");
    }

    out << ":\n";
    indent();
    //print_statements(stmt->get_statements());
    dedent();
    out << '\n';
}*/

/*
void PrettyPrinter::print_if_statement(BranchStatement* stmt) {
    print_indentation();
    out << "if ";

    print_expression(stmt->get_condition());
    out << ":\n";
    indent();
    //print_statement(stmt->get_true_statements());
    dedent();

    if (stmt->get_false_statements()) {
       // print_statement(stmt->get_false_statements());
    } else {
        out << '\n';
    }
}*/

/*
void PrettyPrinter::print_elif_statement(BranchStatement* stmt) {
    print_indentation();
    out << "elif ";

    print_expression(stmt->get_condition());
    out << ":\n";
    indent();
   // print_statement(stmt->get_true_statements());
    dedent();

    if (stmt->get_false_statements()) {
       // print_statement(stmt->get_false_statements());
    } else {
        out << '\n';
    }
}*/

/*
void PrettyPrinter::print_else_statement(BranchStatement* stmt) {
    print_indentation();
    out << "else:\n ";

    indent();
   // print_statement(stmt->get_true_statements());
    dedent();
    out << '\n';
}*/

/*
void PrettyPrinter::print_return_statement(ReturnStatement* stmt) {
    print_indentation();
    out << "return";

    if (stmt->get_expression()) {
        out << ' ';
        print_expression(stmt->get_expression());
    }
}*/

/* Statements */
void PrettyPrinter::print_statements(Ast* stmts) {
    if (stmts->children_count() == 0) {
        print_indentation();
        out << "pass";
        return;
    }

    for (int i = 0; i < stmts->children_count(); ++i) {
        print(stmts->get_child(i));
    }
}

void PrettyPrinter::print_return(Ast* node) {
    print_indentation();
    out << "return";

    Ast* expr = node->get_child();

    if (expr) {
        out << " ";
        print(expr);
    }

    out << "\n";
}

void PrettyPrinter::print_expression(Ast* stmt, bool has_semicolon) {
    print_indentation();
    print(stmt->get_child(0));

    if (has_semicolon) {
        out << ";";
    }

    out << '\n';
}
/*
void PrettyPrinter::print_cast_expression(Cast* expr) {
    print_expression(expr->get_expression());
    out << " as ";
    //print_type(expr->get_type());
}

void PrettyPrinter::print_not_in_expression(NotIn* expr) {
    print_expression(expr->get_left());
    out << " not in ";
    print_expression(expr->get_right());
}
*/
void PrettyPrinter::print_index(Ast* node) {
    print(node->get_child(0));
    out << '[';

    print(node->get_child(1));
    out << ']';
}

void PrettyPrinter::print_hash_pair(Ast* pair) {
    print(pair->get_child(0));
    out << ": ";
    print(pair->get_child(1));
}

void PrettyPrinter::print_logical_not(Ast* un) {
    out << "!";
    print(un->get_child(0));
}

void PrettyPrinter::print_not(Ast* un) {
    out << "not ";
    print(un->get_child(0));
}

void PrettyPrinter::print_address_of(Ast* node) {
    out << "&";
    print(node->get_child());
}

void PrettyPrinter::print_dereference(Ast* node) {
    out << "*";
    print(node->get_child());
}

void PrettyPrinter::print_bitwise_not(Ast* node) {
    out << "~";
    print(node->get_child());
}

void PrettyPrinter::print_unary_minus(Ast* node) {
    out << "-";
    print(node->get_child());
}

void PrettyPrinter::print_unary_plus(Ast* node) {
    out << "+";
    print(node->get_child());
}

void PrettyPrinter::print_pre_increment(Ast* node) {
    out << "++";
    print(node->get_child());
}

void PrettyPrinter::print_pre_decrement(Ast* node) {
    out << "--";
    print(node->get_child());
}

/*
void PrettyPrinter::print_sizeof_expression(UnaryOperator* un) {
    out << "sizeof(";
    print_expression(un->get_expression());
    out << ")";
}

void PrettyPrinter::print_new_expression(New* expr) {
    out << "new ";
    //print_type(expr->get_type());
    //print_expression_list(expr->get_arguments(), "(", ")");
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
}*/

void PrettyPrinter::print_call(Ast* expr) {
    print(expr->get_child(0));
    print(expr->get_child(1));
}

void PrettyPrinter::print_arguments(Ast* args) {
    print_expression_list(args, "(", ")");
}

void PrettyPrinter::print_argument_name(Ast* node) {
    out << node->get_value() << ": ";
    print(node->get_child());
}

void PrettyPrinter::print_pos_increment(Ast* node) {
    print(node->get_child(0));
    out << "++";
}

void PrettyPrinter::print_pos_decrement(Ast* node) {
    print(node->get_child(0));
    out << "--";
}

void PrettyPrinter::print_sizeof(Ast* node) {
    out << "sizeof(";
    print(node->get_child());
    out << ")";
}

/*
void PrettyPrinter::print_expression(Expression* expr) {};

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
}*/

void PrettyPrinter::print_tuple_type(Ast* tuple) {
    print_type_list(tuple, "(", ")");
}

void PrettyPrinter::print_function_type(Ast* type) {
    print_type_list(type->get_child(0), "(", ")");
    out << " -> ";
    print(type->get_child(1));
}

void PrettyPrinter::print_named_type(Ast* type) {
    print(type->get_child(0));
}

void PrettyPrinter::print_array_type(Ast* type) {
    print(type->get_child(0));
    out << "[";
    print(type->get_child(1));
    out << "]";
}

void PrettyPrinter::print_type_list(Ast* tlist, const char* begin, const char* end) {
    if (tlist == nullptr) {
        return;
    }

    int i;

    out << begin;

    for (i = 0; i < tlist->children_count() - 1; ++i) {
        print(tlist->get_child(i));
        out << ", ";
    }

    print(tlist->get_child(i));
    out << end;
}
/*
void PrettyPrinter::print_identifier(Identifier* id) {
    if (id->has_global_alias()) {
        out << "::";
    } else if (id->has_alias()) {
        out << id->get_alias().get_value();
        out << "::";
    }

    out << id->get_name().get_value();
    //print_generics(id->get_generics());
}
*/
void PrettyPrinter::print_generics(Ast* g) {
    print_type_list(g, "<", ">");
}
/*
void PrettyPrinter::print_char_literal(CharLiteral* ch) {
    out << '\'' << ch->get_token().get_value() << '\'';
}

void PrettyPrinter::print_string_literal(StringLiteral* str) {
    char c = '"';

    if (str->get_token().get_kind() == TK_LITERAL_SINGLE_QUOTE_STRING) {
        c = '\'';
    }

    out << c << str->get_token().get_value() << c;
}*/

void PrettyPrinter::print_tuple(Ast* expr) {
    print_expression_list(expr, "(", ")");
}

void PrettyPrinter::print_sequence(Ast* expr) {
    print_expression_list(expr, "(", ")", ";");
}

void PrettyPrinter::print_list(Ast* expr) {
    print_expression_list(expr, "[", "]");
}

void PrettyPrinter::print_array(Ast* expr) {
    print_expression_list(expr, "{", "}");
}

void PrettyPrinter::print_hash(Ast* expr) {
    print_expression_list(expr, "{", "}");
}

void PrettyPrinter::print_expression_list(Ast* list, const char* begin, const char* end, const char* sep) {
    if (list == nullptr) {
        return;
    }

    int i;
    out << begin;

    if (list->children_count() > 0) {
        for (i = 0; i < list->children_count() - 1; ++i) {
            print(list->get_child(i));
            out << sep << " ";
        }

        print(list->get_child(i));
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

void PrettyPrinter::print_binop(Ast* node, const char* oper, bool no_space) {
    //out << "(";
    print(node->get_child(0));

    if (no_space) {
        out << oper;
    } else {
        out << " " << oper << " ";
    }

    print(node->get_child(1));
    //out << ")";
}
