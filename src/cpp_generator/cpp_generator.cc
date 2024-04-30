#include <iostream>
#include "cpp_generator/cpp_generator.h"

using namespace haard;

CppGenerator::CppGenerator() {
    output = nullptr;
    indent_level = 0;
}

std::string CppGenerator::get_output() {
    std::stringstream res;


    res << build_cpp_header();
    res << classes.str();
    res << headers.str() << '\n';
    res << bodies.str();

    return res.str();
}

std::string CppGenerator::build_cpp_header() {
    return "#include <iostream>\n"
           "#include <cstdint>\n\n"
           "typedef uint8_t u8;\n"
           "typedef int8_t i8;\n"
           "typedef uint16_t u16;\n"
           "typedef int16_t i16;\n"
           "typedef uint32_t u32;\n"
           "typedef int32_t i32;\n"
           "typedef uint64_t u64;\n"
           "typedef int64_t i64;\n\n";
}

void CppGenerator::build_modules(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        build_module(modules->get_module(i));
    }
}

void CppGenerator::build_module(Module* module) {
    build_module_classes(module);

    for (int i = 0; i < module->functions_count(); ++i) {
        build_function(module->get_function(i));
    }
}

void CppGenerator::build_module_classes(Module* module) {
    for (int i = 0; i < module->classes_count(); ++i) {
        build_class(module->get_class(i));
    }
}

void CppGenerator::build_class(Class* klass) {
    set_output(&classes);

    *output << "struct " << klass->get_name().get_value() << " {\n";
    indent();

    for (int i = 0; i < klass->variables_count(); ++i) {
        build_member_variable(klass->get_variable(i));
    }

    dedent();
    *output << "};\n\n";
    restore_output();

    for (int i = 0; i < klass->functions_count(); ++i) {
        build_function(klass->get_function(i));
    }
}

void CppGenerator::build_member_variable(Variable* var) {
    print_indentation();
    build_type(var->get_type());
    *output << " " << var->get_name().get_value() << ";\n";
}

void CppGenerator::build_function(Function* function) {
    headers << build_function_header(function) << ";\n";

    build_function_body(function);
}

std::string CppGenerator::build_function_header(Function* function) {
    int i;
    std::stringstream ss;
    Variable* var;

    set_output(&ss);
    build_type(function->get_return_type());
    ss << ' ' << function->get_name().get_value() << '(';

    if (function->is_method()) {
        ss << "void* _this, ";
    }

    if (function->parameters_count() > 0) {
        for (i = 0; i < function->parameters_count() - 1; ++i) {
            var = function->get_parameter(i);
            build_type(var->get_type());
            ss << ' ' << var->get_name().get_value();
            ss << ", ";
        }

        var = function->get_parameter(i);
        build_type(var->get_type());
        ss << ' ' << var->get_name().get_value();
    }

    ss << ')';
    restore_output();
    return ss.str();
}

void CppGenerator::build_function_body(Function* function) {
    std::stringstream ss;

    set_output(&ss);
    *output << build_function_header(function) << " {\n";

    indent();
    build_statement(function->get_statements());
    dedent();
    *output << "}\n\n";
    bodies << output->str();
    restore_output();
}

void CppGenerator::build_statement(Statement* stmt) {
    switch (stmt->get_kind()) {
    case STMT_COMPOUND:
        build_compound_statement((CompoundStatement*) stmt);
        break;

    case STMT_EXPRESSION:
        build_expression_statement((ExpressionStatement*) stmt);
        break;

    case STMT_WHILE:
        build_while_statement((WhileStatement*) stmt);
        break;

    case STMT_FOR:
        build_for_statement((ForStatement*) stmt);
        break;

    case STMT_RETURN:
        build_return_statement((ReturnStatement*) stmt);
        break;

    default:
        //assert(false && "invalid statement");
        break;
    }
}

void CppGenerator::build_while_statement(WhileStatement* stmt) {
    print_indentation();
    *output << "while (";
    build_expression(stmt->get_condition());
    *output << ") {\n";
    indent();
    build_compound_statement(stmt->get_statements());
    dedent();
    print_indentation();
    *output << "}\n";
}

void CppGenerator::build_for_statement(ForStatement* stmt) {

}

void CppGenerator::build_return_statement(ReturnStatement* stmt) {
    print_indentation();
    *output << "return";

    if (stmt->get_expression()) {
        *output << ' ';
        build_expression(stmt->get_expression());
    }

    *output << ";\n";
}

void CppGenerator::build_compound_statement(CompoundStatement* stmt) {
    if (stmt->statements_count() == 0) {
        *output << "/* EMPTY */";
        return;
    }

    for (int i = 0; i < stmt->statements_count(); ++i) {
        build_statement(stmt->get_statement(i));
    }
}

void CppGenerator::build_expression_statement(ExpressionStatement* stmt) {
    print_indentation();
    build_expression(stmt->get_expression());
    *output << ";\n";
}

void CppGenerator::build_expression(Expression* expr) {
    BinaryOperator* bin = (BinaryOperator*) expr;
    UnaryOperator* un = (UnaryOperator*) expr;
    Literal* literal = (Literal*) expr;

    if (expr == nullptr) {
        return;
    }

    switch (expr->get_kind()) {
    case AST_ID:
        build_identifier((Identifier*) expr);
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
        build_binary_operator(bin);
        break;

    case EXPR_DOT:
    case EXPR_ARROW:
        build_binary_operator(bin, true);
        break;

    case EXPR_CAST:
        build_cast_expression((Cast*) expr);
        break;
/*
    case EXPR_NOT_IN:
        print_not_in_expression((NotIn*) expr);
        break;
*/
    case EXPR_INDEX:
        build_index_expression(bin);
        break;

    case EXPR_CALL:
        build_call_expression((Call*) expr);
        break;
/*
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
*/
    case EXPR_UNARY_PLUS:
    case EXPR_UNARY_MINUS:
    case EXPR_ADDRESS_OF:
    case EXPR_DEREFERENCE:
    case EXPR_BITWISE_NOT:
    case EXPR_PRE_INCREMENT:
    case EXPR_PRE_DECREMENT:
        build_unary_operator(un);
        break;

    case EXPR_POS_INCREMENT:
    case EXPR_POS_DECREMENT:
        build_unary_operator(un, true);
        break;

    case EXPR_LITERAL_INTEGER:
    case EXPR_LITERAL_FLOAT:
    case EXPR_LITERAL_DOUBLE:
        *output << literal->get_token().get_value();
        break;

    case EXPR_LITERAL_CHAR:
        build_char_literal((CharLiteral*) expr);
        break;

    case EXPR_LITERAL_STRING:
        build_string_literal((StringLiteral*) expr);
        break;
/*
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
        break;*/
    }
}

void CppGenerator::build_identifier(Identifier* id) {
    *output << id->get_name().get_value();
}

void CppGenerator::build_unary_operator(UnaryOperator* un, bool last) {
    const char* oper = un->get_token().get_value();

    if (last) {
        build_expression(un->get_expression());
        *output << oper;
    } else {
        *output << oper;
        build_expression(un->get_expression());
    }
}

void CppGenerator::build_binary_operator(BinaryOperator* bin, bool no_space) {
    build_expression(bin->get_left());

    const char* oper = bin->get_token().get_value();

    if (no_space) {
        *output << oper;
    } else {
        *output << ' ' << oper << ' ';
    }

    build_expression(bin->get_right());
}

void CppGenerator::build_index_expression(BinaryOperator* bin) {
    build_expression(bin->get_left());
    *output << '[';

    build_expression(bin->get_right());
    *output << ']';
}

void CppGenerator::build_call_expression(Call* expr) {
    build_expression(expr->get_object());

    if (expr->get_arguments()) {
        build_expression_list(expr->get_arguments(), "(", ")");
    } else {
        *output << "()";
    }
}

void CppGenerator::build_cast_expression(Cast* expr) {
    *output << "(";
    build_type(expr->get_type());
    *output << ") ";
    build_expression(expr->get_expression());
}

void CppGenerator::build_char_literal(CharLiteral* ch) {
    *output << '\'' << ch->get_token().get_value() << '\'';
}

void CppGenerator::build_string_literal(StringLiteral* str) {
    *output << '"' << str->get_token().get_value() << '"';
}

void CppGenerator::build_expression_list(ExpressionList *list, const char *begin, const char *end, const char *sep) {
    if (list == nullptr) {
        return;
    }

    int i;
    *output << begin;

    if (list->expressions_count() > 0) {
        for (i = 0; i < list->expressions_count() - 1; ++i) {
            build_expression(list->get_expression(i));
            *output << sep << " ";
        }

        build_expression(list->get_expression(i));
    }

    *output << end;
}

void CppGenerator::build_type(Type* type) {
    switch (type->get_kind()) {
    case TYPE_U8:
        *output << "u8";
        break;

    case TYPE_U16:
        *output << "u16";
        break;

    case TYPE_U32:
        *output << "u32";
        break;

    case TYPE_U64:
        *output << "u64";
        break;

    case TYPE_I8:
        *output << "i8";
        break;

    case TYPE_I16:
        *output << "i16";
        break;

    case TYPE_I32:
        *output << "i32";
        break;

    case TYPE_I64:
        *output << "i64";
        break;

    case TYPE_BOOL:
        *output << "bool";
        break;

    case TYPE_CHAR:
        *output << "i8";
        break;

    case TYPE_UCHAR:
        *output << "u8";
        break;

    case TYPE_SHORT:
        *output << "i16";
        break;

    case TYPE_USHORT:
        *output << "u16";
        break;

    case TYPE_INT:
        *output << "i32";
        break;

    case TYPE_UINT:
        *output << "u32";
        break;

    case TYPE_LONG:
        *output << "i64";
        break;

    case TYPE_ULONG:
        *output << "u64";
        break;

    case TYPE_VOID:
        *output << "void";
        break;
    }
}

void CppGenerator::set_output(std::stringstream* ss) {
    output_stack.push(output);
    output = ss;
}

void CppGenerator::restore_output() {
    output = output_stack.top();
    output_stack.pop();
}

void CppGenerator::indent() {
    ++indent_level;
}

void CppGenerator::dedent() {
    --indent_level;
}

void CppGenerator::print_indentation() {
    for (int i = 0; i < indent_level; ++i) {
        *output << "    ";
    }
}
