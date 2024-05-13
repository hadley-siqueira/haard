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

    case AST_ID:
        build_identifier((Identifier*) expr);
        break;

    case EXPR_LITERAL_BOOLEAN:
        build_boolean_literal((BooleanLiteral*) expr);
        break;

    case EXPR_LITERAL_INTEGER:
        build_integer_literal((IntegerLiteral*) expr);
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

void ExpressionCppGenerator::build_identifier(Identifier* expr) {
    output << expr->get_name().get_value();
}

void ExpressionCppGenerator::build_boolean_literal(BooleanLiteral* expr) {
    output << expr->get_token().get_value();
}

void ExpressionCppGenerator::build_integer_literal(IntegerLiteral* expr) {
    output << expr->get_token().get_value();
}

void ExpressionCppGenerator::build_binop(BinaryOperator* expr, const char* oper) {
    ExpressionCppGenerator gen1;
    ExpressionCppGenerator gen2;

    gen1.build(expr->get_left());
    gen2.build(expr->get_right());

    output << gen1.get_output() << " " << oper << " " << gen2.get_output();
}

std::string ExpressionCppGenerator::get_output() {
    return output.str();
}
