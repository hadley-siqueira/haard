#include <iostream>

#include "cpp_generator/expression_cpp_generator.h"

using namespace haard;

void ExpressionCppGenerator::build(Expression* expr) {
    if (expr == nullptr) {
        return;
    }

    int kind = expr->get_kind();

    switch (kind) {
    case EXPR_ASSIGNMENT:
        build_assignment((Assignment*) expr);
        break;

    case EXPR_PLUS:
        build_plus((Plus*) expr);
        break;

    case EXPR_MINUS:
        build_minus((Minus*) expr);
        break;

    case EXPR_TIMES:
        build_times((Times*) expr);
        break;

    case EXPR_DIVISION:
        build_division((Division*) expr);
        break;

    case EXPR_MODULO:
        build_modulo((Modulo*) expr);
        break;

    case EXPR_INTEGER_DIVISION:
        build_interger_division((IntegerDivision*) expr);
        break;

    case EXPR_BITWISE_OR:
        build_bitwise_or((BitwiseOr*) expr);
        break;

    case EXPR_BITWISE_XOR:
        build_bitwise_xor((BitwiseXor*) expr);
        break;

    case EXPR_BITWISE_AND:
        build_bitwise_and((BitwiseAnd*) expr);
        break;

    case EXPR_SHIFT_LEFT_LOGICAL:
        build_shift_left_logical((ShiftLeftLogical*) expr);
        break;

    case EXPR_SHIFT_RIGHT_LOGICAL:
        build_shift_right_logical((ShiftRightLogical*) expr);
        break;

    case EXPR_SHIFT_RIGHT_ARITHMETIC:
        build_shift_right_arithmetic((ShiftRightArithmetic*) expr);
        break;

    case EXPR_LOGICAL_NOT:
        build_logical_not((LogicalNot*) expr);
        break;

    case EXPR_ADDRESS_OF:
        build_address_of((AddressOf*) expr);
        break;

    case EXPR_DEREFERENCE:
        build_dereference((Dereference*) expr);
        break;

    case EXPR_BITWISE_NOT:
        build_bitwise_not((BitwiseNot*) expr);
        break;

    case EXPR_UNARY_MINUS:
        build_unary_minus((UnaryMinus*) expr);
        break;

    case EXPR_UNARY_PLUS:
        build_unary_plus((UnaryPlus*) expr);
        break;

    case EXPR_DOUBLE_DOLAR:
        build_double_dolar((DoubleDolar*) expr);
        break;

    case EXPR_NEW:
        build_new((New*) expr);
        break;

    case EXPR_DELETE:
        build_delete((Delete*) expr);
        break;

    case EXPR_DELETE_ARRAY:
        build_delete_array((DeleteArray*) expr);
        break;

    case EXPR_DOT:
        build_dot((Dot*) expr);
        break;

    case EXPR_ARROW:
        build_arrow((Arrow*) expr);
        break;

    case EXPR_INDEX:
        build_index((Index*) expr);
        break;

    case EXPR_CALL:
        build_call((Call*) expr);
        break;

    case AST_ID:
        build_identifier((Identifier*) expr);
        break;

    case EXPR_LITERAL_BOOLEAN:
        build_boolean_literal((BooleanLiteral*) expr);
        break;

    case EXPR_LITERAL_INTEGER:
        build_integer_literal((IntegerLiteral*) expr);
        break;

    case EXPR_LITERAL_FLOAT:
        build_float_literal((FloatLiteral*) expr);
        break;

    case EXPR_LITERAL_DOUBLE:
        build_double_literal((DoubleLiteral*) expr);
        break;

    case EXPR_LITERAL_CHAR:
        build_char_literal((CharLiteral*) expr);
        break;

    case EXPR_LITERAL_STRING:
        build_string_literal((StringLiteral*) expr);
        break;

    case EXPR_PARENTHESIS:
        build_parenthesis((Parenthesis*) expr);
        break;

    default:
        output << "expr";
        break;
    }
}

void ExpressionCppGenerator::build_assignment(Assignment* expr) {
    build_binop(expr, "=");
}

void ExpressionCppGenerator::build_plus(Plus* expr) {
    build_binop(expr, "+");
}

void ExpressionCppGenerator::build_minus(Minus* expr) {
    build_binop(expr, "-");
}

void ExpressionCppGenerator::build_times(Times* expr) {
    build_binop(expr, "*");
}

void ExpressionCppGenerator::build_division(Division* expr) {
    build_binop(expr, "/");
}

void ExpressionCppGenerator::build_modulo(Modulo* expr) {
    build_binop(expr, "%");
}

void ExpressionCppGenerator::build_interger_division(IntegerDivision* expr) {
    build_binop(expr, "/ (int)");
}

void ExpressionCppGenerator::build_bitwise_or(BitwiseOr* expr) {
    build_binop(expr, "|");
}

void ExpressionCppGenerator::build_bitwise_xor(BitwiseXor* expr) {
    build_binop(expr, "^");
}

void ExpressionCppGenerator::build_bitwise_and(BitwiseAnd* expr) {
    build_binop(expr, "&");
}

void ExpressionCppGenerator::build_shift_left_logical(ShiftLeftLogical* expr) {
    build_binop(expr, "<<");
}

void ExpressionCppGenerator::build_shift_right_logical(ShiftRightLogical* expr) {
    build_binop(expr, ">>");
}

void ExpressionCppGenerator::build_shift_right_arithmetic(ShiftRightArithmetic* expr) {
    build_binop(expr, ">>");
}

void ExpressionCppGenerator::build_logical_not(LogicalNot* expr) {
    build_unary(expr, "!");
}

void ExpressionCppGenerator::build_address_of(AddressOf* expr) {
    build_unary(expr, "&");
}

void ExpressionCppGenerator::build_dereference(Dereference* expr) {
    build_unary(expr, "*");
}

void ExpressionCppGenerator::build_bitwise_not(BitwiseNot* expr) {
    build_unary(expr, "~");
}

void ExpressionCppGenerator::build_unary_minus(UnaryMinus* expr) {
    build_unary(expr, "-");
}

void ExpressionCppGenerator::build_unary_plus(UnaryPlus* expr) {
    build_unary(expr, "+");
}

void ExpressionCppGenerator::build_double_dolar(DoubleDolar* expr) {
    StringLiteral* s = (StringLiteral*) expr->get_expression();
    std::string t = s->get_token().get_value();
    output << t;
}

void ExpressionCppGenerator::build_new(New* expr) {
    output << "new ";
    output << expr->get_type()->to_cpp();

    if (expr->get_arguments()) {
        int i;

        output << "(";

        for (i = 0; i < expr->get_arguments()->expressions_count() - 1; ++i) {
            ExpressionCppGenerator gen;

            gen.build(expr->get_arguments()->get_expression(i));
            output << gen.get_output() << ", ";
        }

        ExpressionCppGenerator gen;

        gen.build(expr->get_arguments()->get_expression(i));
        output << gen.get_output() << ")";
    }
}

void ExpressionCppGenerator::build_delete(Delete* expr) {
    ExpressionCppGenerator gen;

    gen.build(expr->get_expression());
    output << "delete " << gen.get_output();
}

void ExpressionCppGenerator::build_delete_array(DeleteArray* expr) {
    ExpressionCppGenerator gen;

    gen.build(expr->get_expression());
    output << "delete[] " << gen.get_output();
}

void ExpressionCppGenerator::build_dot(Dot* expr) {
    build_binop(expr, ".", true);
}

void ExpressionCppGenerator::build_arrow(Arrow* expr) {
    build_binop(expr, "->", true);
}

void ExpressionCppGenerator::build_index(Index* expr) {
    ExpressionCppGenerator g1;
    ExpressionCppGenerator g2;

    g1.build(expr->get_left());
    g2.build(expr->get_right());

    output << g1.get_output() << '[' << g2.get_output() << ']';
}

void ExpressionCppGenerator::build_call(Call* expr) {
    ExpressionCppGenerator gen;

    gen.build(expr->get_object());
    output << gen.get_output() << "(";

    if (expr->get_arguments()->expressions_count() > 0) {
        int i;

        for (i = 0; i < expr->get_arguments()->expressions_count() - 1; ++i) {
            ExpressionCppGenerator g;

            g.build(expr->get_arguments()->get_expression(i));
            output << g.get_output() << ", ";
        }

        ExpressionCppGenerator g;
        g.build(expr->get_arguments()->get_expression(i));
        output << g.get_output();
    }

    output << ")";
}

void ExpressionCppGenerator::build_identifier(Identifier* expr) {
    output << expr->get_name().get_value();
}

void ExpressionCppGenerator::build_boolean_literal(BooleanLiteral* expr) {
    output << expr->get_token().get_value();
}

void ExpressionCppGenerator::build_integer_literal(IntegerLiteral* expr) {
    output << expr->get_token().get_value();
}

void ExpressionCppGenerator::build_float_literal(FloatLiteral* expr) {
    output << expr->get_token().get_value();
}

void ExpressionCppGenerator::build_double_literal(DoubleLiteral* expr) {
    output << expr->get_token().get_value();
}

void ExpressionCppGenerator::build_char_literal(CharLiteral* expr) {
    output << "'" << expr->get_token().get_value() << "'";
}

void ExpressionCppGenerator::build_string_literal(StringLiteral* expr) {
    output << '"' << expr->get_token().get_value() << '"';
}

void ExpressionCppGenerator::build_parenthesis(Parenthesis* expr) {
    ExpressionCppGenerator gen;

    gen.build(expr->get_expression());
    output << "(" << gen.get_output() << ")";
}

void ExpressionCppGenerator::build_binop(BinaryOperator* expr, const char* oper, bool no_space) {
    ExpressionCppGenerator gen1;
    ExpressionCppGenerator gen2;

    gen1.build(expr->get_left());
    gen2.build(expr->get_right());

    if (no_space) {
        output << gen1.get_output() << oper << gen2.get_output();
    } else {
        output << gen1.get_output() << " " << oper << " " << gen2.get_output();
    }
}

void ExpressionCppGenerator::build_unary(UnaryOperator* expr, const char* oper, bool last) {
    ExpressionCppGenerator gen;

    gen.build(expr->get_expression());

    if (last) {
        output << gen.get_output() << oper;
    } else {
        output << oper << gen.get_output();
    }
}

std::string ExpressionCppGenerator::get_output() {
    return output.str();
}
