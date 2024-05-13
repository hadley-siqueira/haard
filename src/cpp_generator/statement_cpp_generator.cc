#include "cpp_generator/statement_cpp_generator.h"
#include "cpp_generator/expression_cpp_generator.h"
#include "utils/utils.h"

using namespace haard;

void StatementCppGenerator::build(Statement* stmt) {
    if (stmt == nullptr) {
        return;
    }

    switch (stmt->get_kind()) {
    case STMT_COMPOUND:
        build_compound_statement((CompoundStatement*) stmt);
        break;

    case STMT_WHILE:
        build_while_statement((WhileStatement*) stmt);
        break;

    case STMT_EXPRESSION:
        build_expression_statement((ExpressionStatement*) stmt);
        break;
    }
}

void StatementCppGenerator::build_compound_statement(CompoundStatement* stmt) {
    for (int i = 0; i < stmt->statements_count(); ++i) {
        StatementCppGenerator gen;

        gen.build(stmt->get_statement(i));
        output << gen.get_output();

        if (i < stmt->statements_count() - 1) {
            output << '\n';
        }
    }
}

void StatementCppGenerator::build_while_statement(WhileStatement* stmt) {
    StatementCppGenerator gen;
    ExpressionCppGenerator expr_gen;

    expr_gen.build(stmt->get_condition());
    gen.build(stmt->get_statements());

    output << "while (" << expr_gen.get_output() << ") {\n";
    output << indent(gen.get_output(), 4) << "\n}";
}

void StatementCppGenerator::build_expression_statement(ExpressionStatement* stmt) {
    ExpressionCppGenerator gen;

    gen.build(stmt->get_expression());
    output << gen.get_output() << ";";
}

std::string StatementCppGenerator::get_output() {
    return output.str();
}
