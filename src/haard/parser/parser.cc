#include <iostream>
#include <sstream>
#include <cassert>

#include "haard/parser/parser.h"
#include "haard/scanner/scanner.h"
#include "haard/log/logs.h"

#include "haard/ast/types/primitive_type.h"
#include "haard/ast/types/pointer_type.h"
#include "haard/ast/types/reference_type.h"
#include "haard/ast/types/list_type.h"
#include "haard/ast/types/array_type.h"
#include "haard/ast/types/tuple_type.h"
#include "haard/ast/types/function_type.h"
#include "haard/ast/types/named_type.h"

#include "haard/ast/statements/expression_statement.h"
#include "haard/ast/expressions/operators/binary/binary_operator.h"
#include "haard/ast/expressions/operators/binary/assignment.h"
#include "haard/ast/expressions/operators/binary/bitwise_and_assignment.h"
#include "haard/ast/expressions/operators/binary/bitwise_or_assignment.h"
#include "haard/ast/expressions/operators/binary/bitwise_xor_assignment.h"
#include "haard/ast/expressions/operators/binary/bitwise_not_assignment.h"
#include "haard/ast/expressions/operators/binary/division_assignment.h"
#include "haard/ast/expressions/operators/binary/integer_division_assignment.h"
#include "haard/ast/expressions/operators/binary/minus_assignment.h"
#include "haard/ast/expressions/operators/binary/modulo_assignment.h"
#include "haard/ast/expressions/operators/binary/plus_assignment.h"
#include "haard/ast/expressions/operators/binary/times_assignment.h"
#include "haard/ast/expressions/operators/binary/shift_left_logical_assignment.h"
#include "haard/ast/expressions/operators/binary/shift_right_logical_assignment.h"
#include "haard/ast/expressions/operators/binary/shift_right_arithmetic_assignment.h"
#include "haard/ast/expressions/operators/binary/logical_and.h"
#include "haard/ast/expressions/operators/binary/logical_or.h"
#include "haard/ast/expressions/operators/binary/equal.h"
#include "haard/ast/expressions/operators/binary/not_equal.h"
#include "haard/ast/expressions/operators/binary/less_than.h"
#include "haard/ast/expressions/operators/binary/greater_than.h"
#include "haard/ast/expressions/operators/binary/less_than_or_equal.h"
#include "haard/ast/expressions/operators/binary/greater_than_or_equal.h"
#include "haard/ast/expressions/operators/binary/in.h"
#include "haard/ast/expressions/operators/binary/not_in.h"
#include "haard/ast/expressions/operators/binary/inclusive_range.h"
#include "haard/ast/expressions/operators/binary/exclusive_range.h"
#include "haard/ast/expressions/operators/binary/plus.h"
#include "haard/ast/expressions/operators/binary/minus.h"
#include "haard/ast/expressions/operators/binary/times.h"
#include "haard/ast/expressions/operators/binary/division.h"
#include "haard/ast/expressions/operators/binary/modulo.h"
#include "haard/ast/expressions/operators/binary/integer_division.h"
#include "haard/ast/expressions/operators/binary/power.h"
#include "haard/ast/expressions/operators/binary/shift_left_logical.h"
#include "haard/ast/expressions/operators/binary/shift_right_logical.h"
#include "haard/ast/expressions/operators/binary/shift_right_arithmetic.h"
#include "haard/ast/expressions/operators/binary/bitwise_and.h"
#include "haard/ast/expressions/operators/binary/bitwise_or.h"
#include "haard/ast/expressions/operators/binary/bitwise_xor.h"
#include "haard/ast/expressions/operators/binary/index.h"
#include "haard/ast/expressions/operators/binary/dot.h"
#include "haard/ast/expressions/operators/binary/arrow.h"
#include "haard/ast/expressions/operators/binary/generics_application.h"
#include "haard/ast/expressions/operators/binary/scope.h"

#include "haard/ast/expressions/operators/unary/unary_operator.h"
#include "haard/ast/expressions/operators/unary/unary_plus.h"
#include "haard/ast/expressions/operators/unary/unary_minus.h"
#include "haard/ast/expressions/operators/unary/address_of.h"
#include "haard/ast/expressions/operators/unary/dereference.h"
#include "haard/ast/expressions/operators/unary/bitwise_not.h"
#include "haard/ast/expressions/operators/unary/logical_not.h"
#include "haard/ast/expressions/operators/unary/pre_increment.h"
#include "haard/ast/expressions/operators/unary/pre_decrement.h"
#include "haard/ast/expressions/operators/unary/pos_increment.h"
#include "haard/ast/expressions/operators/unary/pos_decrement.h"
#include "haard/ast/expressions/operators/unary/parenthesis.h"

#include "haard/ast/expressions/literals/boolean_literal.h"
#include "haard/ast/expressions/literals/char_literal.h"
#include "haard/ast/expressions/literals/integer_literal.h"
#include "haard/ast/expressions/literals/float_literal.h"
#include "haard/ast/expressions/literals/string_literal.h"
#include "haard/ast/expressions/literals/symbol_literal.h"

#include "haard/ast/expressions/this.h"
#include "haard/ast/expressions/null.h"
#include "haard/ast/expressions/tuple.h"
#include "haard/ast/expressions/sequence.h"
#include "haard/ast/expressions/list.h"

using namespace haard;

Parser::Parser() {
    idx = 0;
}

Module* Parser::read(std::string path, std::string relative_path) {
    Module* module;
    Scanner sc;

    idx = 0;
    tokens = sc.read(path);
    module = parse_module();

    return module;
}

Module* Parser::parse_module() {
    Module* module = new Module();

    while (true) {
        if (lookahead(TK_IMPORT)) {
            module->add_import(parse_import());
        } else if (lookahead(TK_DEF)) {
            module->add_function(parse_function());
        } else if (lookahead(TK_CLASS)) {
            module->add_child(parse_user_type());
        } else if (lookahead(TK_STRUCT)) {
            module->add_child(parse_user_type());
        } else if (lookahead(TK_UNION)) {
            module->add_child(parse_user_type());
        } else if (lookahead(TK_ENUM)) {
            module->add_child(parse_user_type());
        } else if (lookahead(TK_VAR)) {
            module->add_child(parse_variable_definition());
        } else {
            break;
        }
    }

    return module;
}

Import* Parser::parse_import() {
    Import* import = new Import();

    expect(TK_IMPORT);
    import->set_from_token(matched);

    do {
        expect(TK_ID);
        import->add_to_path(matched);
    } while (match(TK_DOT));

    if (match(TK_AS)) {
        expect(TK_ID);
        import->set_alias(matched);
    }

    return import;
}

Ast* Parser::parse_user_type() {
    bool has_children = false;
    Ast* user_type = nullptr;

    if (match(TK_CLASS)) {
        user_type = new Ast(AST_CLASS);
    } else if (match(TK_STRUCT)) {
        user_type = new Ast(AST_STRUCT);
    } else if (match(TK_UNION)) {
        user_type = new Ast(AST_UNION);
    } else if (match(TK_ENUM)) {
        user_type = new Ast(AST_ENUM);
    } else {
        log_error("missing user type builder");
        return nullptr;
    }

    expect(TK_ID);
    user_type->set_from_token(matched);
    user_type->add_child(parse_generics());

    if (match(TK_LEFT_PARENTHESIS)) {
        Ast* type = parse_type();

        if (type == nullptr) {
            log_error("missing super type on type definition");
        } else {
            user_type->add_child(AST_SUPER, type);
        }

        expect(TK_RIGHT_PARENTHESIS);
    }

    expect(TK_COLON);
    indent();

    while (is_indented()) {
        if (lookahead(TK_DEF)) {
            has_children = true;
            user_type->add_child(parse_function());
        } else if (lookahead(TK_ID)) {
            has_children = true;
            user_type->add_child(parse_field());
        } else if (match(TK_PASS)) {
            if (has_children) {
                log_error("unexpected pass because fields were already declared");
            }

            break;
        } else {
            break;
        }
    }

    dedent();
    return user_type;
}

Ast* Parser::parse_field() {
    Ast* var = new Ast(AST_VARIABLE);

    expect(TK_ID);
    var->set_from_token(matched);

    if (match(TK_COLON)) {
        var->add_child(AST_TYPE, parse_type());
    }

    if (match(TK_ASSIGNMENT)) {
        Ast* expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing field");
        } else {
            var->add_child(AST_EXPRESSION, expr);
        }
    }

    return var;
}

Ast* Parser::parse_variable_definition() {
    bool need_expression = true;
    Ast* var = new Ast(AST_VARIABLE);

    expect(TK_VAR);
    expect(TK_ID);
    var->set_from_token(matched);

    if (match(TK_COLON)) {
        Ast* type = parse_type();
        need_expression = false;

        if (type == nullptr) {
            log_error("missing type on variable definition");
        } else {
            var->add_child(AST_TYPE, type);
        }
    }

    if (need_expression) {
        expect(TK_ASSIGNMENT);
    } else {
        if (match(TK_ASSIGNMENT)) {
            need_expression = true;
        }
    }

    if (need_expression) {
        Ast* expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing expression on variable definition");
        } else {
            var->add_child(AST_EXPRESSION, expr);
        }
    }

    return var;
}

Function* Parser::parse_function() {
    Function* function = new Function();

    expect(TK_DEF);
    expect(TK_ID);
    function->set_name(matched);
    function->set_generics(parse_generics());

    expect(TK_COLON);
    indent();

    Type* return_type = parse_type();

    if (return_type) {
        function->set_return_type(return_type);
    } else {
        log_error("Expected return type in function " + function->get_value());
    }

    while (has_parameters()) {
        function->add_parameter(parse_parameter());
    }

    function->set_statements(parse_statements());
    dedent();

    return function;
}

Variable* Parser::parse_parameter() {
    Variable* param = new Variable();

    expect(TK_AT);
    expect(TK_ID);
    param->set_name(matched);

    expect(TK_COLON);
    Type* type = parse_type();

    if (type != nullptr) {
        param->set_type(type);
    } else {
        log_error("expected type in parameter");
    }

    if (match(TK_ASSIGNMENT)) {
        Ast* expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing expression on default parameter value");
        }

        param->set_expression(expr);
    }

    return param;
}

Statement* Parser::parse_statement() {
    Statement* stmt = nullptr;
    Expression* expr = nullptr;

    if (lookahead(TK_WHILE)) {
        stmt = parse_while_statement();
    } else if (lookahead(TK_FOR)) {
        //stmt = parse_for_statement();
    } else if (lookahead(TK_IF)) {
        //stmt = parse_if_statement();
    } else if (lookahead(TK_RETURN)) {
        stmt = parse_return_statement();
    } else if (lookahead(TK_SWITCH)) {
        //stmt = parse_switch_statement();
    } else if (lookahead(TK_VAR)) {
        //stmt = parse_variable_definition();
    } else {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("expected an expression statement");
        } else {
            stmt = new ExpressionStatement(expr);
        }

        match(TK_SEMICOLON);
    }

    return stmt;
}

WhileStatement* Parser::parse_while_statement() {
    Expression* condition = nullptr;
    WhileStatement* stmt = new WhileStatement();

    expect(TK_WHILE);
    stmt->set_token(matched);
    condition = parse_expression();

    if (condition == nullptr) {
        log_error("expected while condition");
    } else {
        stmt->set_expression(condition);
    }

    expect(TK_COLON);
    indent();
    stmt->set_statements(parse_statements());
    dedent();

    return stmt;
}

Ast* Parser::parse_for_statement() {
    Ast* stmt = new Ast(AST_FOR);
    Ast* expr = nullptr;
    bool is_range = false;

    expect(TK_FOR);

    if (!match(TK_SEMICOLON)) {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("error while parsing for range or init expression");
        }
    }

    if (expr != nullptr && expr->get_kind() == AST_IN) {
        stmt->add_child(AST_FOR_RANGE, expr);
        is_range = true;
    } else if (expr != nullptr) {
        Ast* init = new Ast(AST_FOR_INIT);
        init->add_child(expr);

        while (match(TK_COMMA)) {
            expr = parse_expression();

            if (expr == nullptr) {
                log_error("missing expression in for init list");
            }

            init->add_child(expr);
        }

        stmt->add_child(init);
        expect(TK_SEMICOLON);
    }

    if (!is_range && !match(TK_SEMICOLON)) {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing test expression in for loop");
        }

        stmt->add_child(AST_FOR_TEST, expr);
        expect(TK_SEMICOLON);
    }

    if (!is_range && !lookahead(TK_COLON)) {
        Ast* update = new Ast(AST_FOR_UPDATE);

        do {
            expr = parse_expression();

            if (expr == nullptr) {
                log_error("missing expression in for update expression");
            }

            update->add_child(expr);
        } while (match(TK_COMMA));

        stmt->add_child(update);
    }

    expect(TK_COLON);
    indent();
    stmt->add_child(parse_statements());
    dedent();

    return stmt;
}

Ast* Parser::parse_if_statement() {
    Ast* stmt;
    Ast* expr;
    bool has_expr = true;

    if (match(TK_IF)) {
        stmt = new Ast(AST_IF, matched);
    } else if (match(TK_ELIF)) {
        stmt = new Ast(AST_ELIF, matched);
    } else if (match(TK_ELSE)) {
        stmt = new Ast(AST_ELSE, matched);
        has_expr = false;
    }

    if (has_expr) {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing condition in if statement");
        }

        stmt->add_child(expr);
    }

    expect(TK_COLON);
    indent();
    stmt->add_child(parse_statements());
    dedent();

    if (lookahead(TK_ELIF) && is_indented()) {
        stmt->add_child(parse_if_statement());
    } else if (lookahead(TK_ELSE) && is_indented() && stmt->get_kind() != AST_ELSE) {
        stmt->add_child(parse_if_statement());
    }

    return stmt;
}

Ast* Parser::parse_elif_statement() {/*
    Expression* condition;
    BranchStatement* stmt = new BranchStatement(STMT_ELIF);

    expect(TK_ELIF);
    stmt->set_token(matched);
    condition = parse_expression();

    if (condition == nullptr) {
        log_error("missing condition in elif");
    }

    stmt->set_condition(condition);

    expect(TK_COLON);
    indent();
    stmt->set_true_statements(parse_compound_statement());
    dedent();

    if (lookahead(TK_ELIF) && is_indented()) {
        stmt->set_false_statements(parse_elif_statement());
    } else if (lookahead(TK_ELSE) && is_indented()) {
        stmt->set_false_statements(parse_else_statement());
    }

    return stmt;*/
    return nullptr;
}

Ast* Parser::parse_else_statement() {/*
    BranchStatement* stmt = new BranchStatement(STMT_ELSE);

    expect(TK_ELSE);
    stmt->set_token(matched);

    expect(TK_COLON);
    indent();
    stmt->set_true_statements(parse_compound_statement());
    dedent();

    return stmt;*/
    return nullptr;
}


ReturnStatement* Parser::parse_return_statement() {
    ReturnStatement* stmt = nullptr;
    Expression* expression = nullptr;

    if (!match(TK_RETURN)) {
        return nullptr;
    }

    stmt = new ReturnStatement(matched);

    if (next_token_on_same_line()) {
        expression = parse_expression();

        if (expression == nullptr) {
            log_error("expected an expression in return statement, but got something else");
        } else {
            stmt->set_expression(expression);
        }
    }

    return stmt;
}

Ast* Parser::parse_switch_statement() {
    Ast* stmt = new Ast(AST_SWITCH);
    Ast* expr;

    expect(TK_SWITCH);
    stmt->set_from_token(matched);
    expr = parse_expression();

    if (expr == nullptr) {
        log_error("missing expression on switch statement");
    } else {
        stmt->add_child(expr);
    }

    expect(TK_COLON);
    indent();

    while (true) {
        if (lookahead(TK_CASE)) {
            Ast* cases = parse_switch_cases();
            Ast* brace = new Ast(AST_SWITCH_BRACE);

            brace->add_child(cases);
            indent();
            brace->add_child(parse_statements());
            dedent();
            stmt->add_child(brace);
        } else if (match(TK_DEFAULT)) {
            Ast* cs = new Ast(AST_SWITCH_DEFAULT);
            expect(TK_COLON);
            indent();
            cs->add_child(parse_statements());
            dedent();
            stmt->add_child(cs);
        } else {
            //expect(TK_PASS);
            break;
        }
    }

    dedent();
    return stmt;
}

Ast* Parser::parse_switch_cases() {
    Ast* node = new Ast(AST_SWITCH_CASES);

    while (match(TK_CASE)) {
        Ast* cs = new Ast(AST_SWITCH_CASE, matched);
        Ast* expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing expression on case");
        }

        cs->add_child(expr);
        expect(TK_COLON);
        node->add_child(cs);
    }

    return node;
}

Statements* Parser::parse_statements() {
    Statements* statements = new Statements();

    if (match(TK_PASS)) {
        return statements;
    }

    while (is_indented() && !lookahead(TK_RIGHT_CURLY_BRACKET)) {
        statements->add_statement(parse_statement());
    }

    return statements;
}

Type* Parser::parse_type() {
    return parse_tuple_or_function_type();
}

Type* Parser::parse_tuple_or_function_type() {
    Type* type = nullptr;
    std::vector<Type*> types;

    if (!lookahead(TK_LEFT_PARENTHESIS)) {
        return parse_primary_type();
    }

    expect(TK_LEFT_PARENTHESIS);

    do {
        type = parse_type();

        if (type == nullptr) {
            log_error("type can't be null on tuple or function type");
        } else {
            types.push_back(type);
        }
    } while (match(TK_COMMA));

    expect(TK_RIGHT_PARENTHESIS);

    if (match(TK_ARROW)) {
        if (!next_token_on_same_line()) {
            assert(false && "return type should be on same line");
        }

        Type* return_type = parse_type();

        if (return_type == nullptr) {
            log_error("missing return type on function type");
        } else {
            type = new FunctionType(types, return_type);
        }
    } else {
        type = new TupleType(types);
    }

    return type;
}

Type* Parser::parse_primary_type() {
    Type* type = nullptr;
    //Type* subtype = nullptr;

    if (match(TK_CHAR)) {
       type = new PrimitiveType(AST_TYPE_CHAR, matched);
    } else if (match(TK_SYMBOL)) {
       type = new PrimitiveType(AST_TYPE_SYMBOL, matched);
    } else if (match(TK_VOID)) {
       type = new PrimitiveType(AST_TYPE_VOID, matched);
    } else if (match(TK_BOOL)) {
       type = new PrimitiveType(AST_TYPE_BOOL, matched);
    } else if (match(TK_STR)) {
       type = new PrimitiveType(AST_TYPE_STR, matched);
    } else if (match(TK_I8)) {
       type = new PrimitiveType(AST_TYPE_I8, matched);
    } else if (match(TK_I16)) {
       type = new PrimitiveType(AST_TYPE_I16, matched);
    } else if (match(TK_I32)) {
       type = new PrimitiveType(AST_TYPE_I32, matched);
    } else if (match(TK_I64)) {
       type = new PrimitiveType(AST_TYPE_I64, matched);
    } else if (match(TK_U8)) {
       type = new PrimitiveType(AST_TYPE_U8, matched);
    } else if (match(TK_U16)) {
       type = new PrimitiveType(AST_TYPE_U16, matched);
    } else if (match(TK_U32)) {
       type = new PrimitiveType(AST_TYPE_U32, matched);
    } else if (match(TK_U64)) {
       type = new PrimitiveType(AST_TYPE_U64, matched);
    } else if (match(TK_F32)) {
       type = new PrimitiveType(AST_TYPE_F32, matched);
    } else if (match(TK_F64)) {
       type = new PrimitiveType(AST_TYPE_F64, matched);
    } else if (match(TK_LEFT_SQUARE_BRACKET)) {
        type = parse_type();

        if (type == nullptr) {
            log_error("missing subtype on list type");
        } else {
            type = new ListType(type);
        }

        expect(TK_RIGHT_SQUARE_BRACKET);
    } else if (lookahead(TK_ID) || lookahead(TK_SCOPE)) {
        type = parse_named_type();
    }

    while (type != nullptr) {
        if (match_same_line(TK_TIMES)) {
            type = new PointerType(type, matched);
        } else if (match_same_line(TK_POWER)) {
            type = new PointerType(type, matched);
            type = new PointerType(type, matched);
        } else if (match_same_line(TK_BITWISE_AND)) {
            type = new ReferenceType(type, matched);
        } else if (match_same_line(TK_LOGICAL_AND)) {
            type = new ReferenceType(type, matched);
            type = new ReferenceType(type, matched);
        } else if (match_same_line(TK_LEFT_SQUARE_BRACKET)) {
            auto array = new ArrayType(type, matched);

            if (!lookahead(TK_RIGHT_SQUARE_BRACKET)) {
                array->set_expression(parse_expression());
            }

            type = array;
            expect(TK_RIGHT_SQUARE_BRACKET);
        } else {
            break;
        }
    }

    return type;
}

Type* Parser::parse_named_type() {
    Expression* name = parse_generic_application();

    if (name == nullptr) {
        return nullptr;
    }

    return new NamedType(name);
}

Expression* Parser::parse_expression() {
    return (Expression*) parse_assignment_expression();
}

Expression* Parser::parse_assignment_expression() {
    Expression* expr = parse_cast_expression();

    while (true) {
        if (match(TK_ASSIGNMENT)) {
            Assignment* oper = new Assignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on = operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_BITWISE_AND_ASSIGNMENT)) {
            BitwiseAndAssignment* oper = new BitwiseAndAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on &= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_BITWISE_XOR_ASSIGNMENT)) {
            BitwiseXorAssignment* oper = new BitwiseXorAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on ^= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_BITWISE_OR_ASSIGNMENT)) {
            BitwiseOrAssignment* oper = new BitwiseOrAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on |= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_BITWISE_NOT_ASSIGNMENT)) {
            BitwiseNotAssignment* oper = new BitwiseNotAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on ~= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_DIVISION_ASSIGNMENT)) {
            DivisionAssignment* oper = new DivisionAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on /= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_INTEGER_DIVISION_ASSIGNMENT)) {
            IntegerDivisionAssignment* oper = new IntegerDivisionAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on //= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_MINUS_ASSIGNMENT)) {
            Minus* oper = new Minus(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on -= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_MODULO_ASSIGNMENT)) {
            Modulo* oper = new Modulo(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on %= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_PLUS_ASSIGNMENT)) {
            Plus* oper = new Plus(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on += operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_TIMES_ASSIGNMENT)) {
            Times* oper = new Times(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on *= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_SLL_ASSIGNMENT)) {
            ShiftLeftLogicalAssignment* oper = new ShiftLeftLogicalAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on <<= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_SRA_ASSIGNMENT)) {
            ShiftRightArithmeticAssignment* oper = new ShiftRightArithmeticAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on >>= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_SRL_ASSIGNMENT)) {
            ShiftRightLogicalAssignment* oper = new ShiftRightLogicalAssignment(matched, expr);
            expr = parse_assignment_expression();

            if (expr == nullptr) {
                log_error("missing rhs on >>>= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_cast_expression() {
    Ast* type = nullptr;
    Ast* subexpr;
    Ast* expr = parse_logical_or_expression();

    if (expr != nullptr && match(TK_AS)) {
        subexpr = expr;
        expr = new Ast(AST_CAST, matched);

        if (next_token_on_same_line()) {
            type = parse_type();
        }

        if (type == nullptr) {
            log_error("missing type on cast expression");
        }

        expr->add_child(subexpr);
        expr->add_child(type);
    }

    return (Expression*) expr;
}

Expression* Parser::parse_logical_or_expression() {
    Expression* expr = parse_logical_and_expression();

    while (true) {
        if (match(TK_LOGICAL_OR)) {
            auto* oper = new LogicalOr(matched, expr);
            expr = parse_logical_and_expression();

            if (expr == nullptr) {
                log_error("missing rhs on 'or' operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_OR)) {
            auto* oper = new LogicalOr(matched, expr);
            expr = parse_logical_and_expression();

            if (expr == nullptr) {
                log_error("missing rhs on || operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_logical_and_expression() {
    Expression* expr = parse_equality_expression();

    while (true) {
        if (match(TK_LOGICAL_AND)) {
            auto* oper = new LogicalAnd(matched, expr);
            expr = parse_equality_expression();

            if (expr == nullptr) {
                log_error("missing rhs on 'and' operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_AND)) {
            auto* oper = new LogicalAnd(matched, expr);
            expr = parse_equality_expression();

            if (expr == nullptr) {
                log_error("missing rhs on && operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_equality_expression() {
    Expression* expr = parse_relational_expression();

    while (true) {
        if (match(TK_EQ)) {
            auto* oper = new Equal(matched, expr);
            expr = parse_relational_expression();

            if (expr == nullptr) {
                log_error("missing rhs on == operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_NE)) {
            auto* oper = new NotEqual(matched, expr);
            expr = parse_relational_expression();

            if (expr == nullptr) {
                log_error("missing rhs on != operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_relational_expression() {
    Expression* expr = parse_range_expression();

    while (true) {
        if (match(TK_LT)) {
            auto* oper = new LessThan(matched, expr);
            expr = parse_range_expression();

            if (expr == nullptr) {
                log_error("missing rhs on < operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_GT)) {
            auto* oper = new GreaterThan(matched, expr);
            expr = parse_range_expression();

            if (expr == nullptr) {
                log_error("missing rhs on > operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_LE)) {
            auto* oper = new LessThanOrEqual(matched, expr);
            expr = parse_range_expression();

            if (expr == nullptr) {
                log_error("missing rhs on <= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_GE)) {
            auto* oper = new GreaterThanOrEqual(matched, expr);
            expr = parse_range_expression();

            if (expr == nullptr) {
                log_error("missing rhs on >= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_IN)) {
            auto* oper = new In(matched, expr);
            expr = parse_range_expression();

            if (expr == nullptr) {
                log_error("missing rhs on 'in' operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_NOT)) {
            expect(TK_IN);
            auto* oper = new NotIn(matched, expr);
            expr = parse_range_expression();

            if (expr == nullptr) {
                log_error("missing rhs on >= operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_range_expression() {
    Expression* expr = parse_arith_expression();

    while (true) {
        if (match(TK_INCLUSIVE_RANGE)) {
            auto* oper = new InclusiveRange(matched, expr);
            expr = parse_arith_expression();

            if (expr == nullptr) {
                log_error("missing rhs on .. operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_EXCLUSIVE_RANGE)) {
            auto* oper = new ExclusiveRange(matched, expr);
            expr = parse_arith_expression();

            if (expr == nullptr) {
                log_error("missing rhs on ... operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_arith_expression() {
    Expression* expr = parse_term_expression();

    while (true) {
        if (match_same_line(TK_PLUS)) {
            Plus* oper = new Plus(matched, expr);
            expr = parse_term_expression();

            if (expr == nullptr) {
                log_error("missing rhs on + operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match_same_line(TK_MINUS)) {
            Minus* oper = new Minus(matched, expr);

            expr = parse_term_expression();

            if (expr == nullptr) {
                log_error("missing rhs on - operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_term_expression() {
    Expression* expr = parse_power_expression();

    while (true) {
        if (match_same_line(TK_TIMES)) {
            Times* oper = new Times(matched, expr);
            expr = parse_power_expression();

            if (expr == nullptr) {
                log_error("missing rhs on * operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match_same_line(TK_DIVISION)) {
            Division* oper = new Division(matched, expr);
            expr = parse_power_expression();

            if (expr == nullptr) {
                log_error("missing rhs on / operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match_same_line(TK_MODULO)) {
            Modulo* oper = new Modulo(matched, expr);
            expr = parse_power_expression();

            if (expr == nullptr) {
                log_error("missing rhs on % operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match_same_line(TK_INTEGER_DIVISION)) {
            IntegerDivision* oper = new IntegerDivision(matched, expr);
            expr = parse_power_expression();

            if (expr == nullptr) {
                log_error("missing rhs on // operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_power_expression() {
    Expression* expr = parse_bitwise_or_expression();

    if (match(TK_POWER)) {
        Power* oper = new Power(matched, expr);
        expr = parse_power_expression();

        if (expr == nullptr) {
            log_error("missing rhs on ** operator");
        } else {
            oper->set_right(expr);
        }

        expr = oper;
    }

    return expr;
}

Expression* Parser::parse_bitwise_or_expression() {
    Expression* expr = parse_bitwise_xor_expression();

    while (true) {
        if (match(TK_BITWISE_OR)) {
            BitwiseOr* oper = new BitwiseOr(matched, expr);
            expr = parse_bitwise_xor_expression();

            if (expr == nullptr) {
                log_error("missing rhs on | operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_bitwise_xor_expression() {
    Expression* expr = parse_bitwise_and_expression();

    while (true) {
        if (match(TK_BITWISE_XOR)) {
            BitwiseXor* oper = new BitwiseXor(matched, expr);
            expr = parse_bitwise_and_expression();

            if (expr == nullptr) {
                log_error("missing rhs on ^ operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_bitwise_and_expression() {
    Expression* expr = parse_shift_expression();

    while (true) {
        if (match_same_line(TK_BITWISE_AND)) {
            BitwiseAnd* oper = new BitwiseAnd(matched, expr);
            expr = parse_shift_expression();

            if (expr == nullptr) {
                log_error("missing rhs on & operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_shift_expression() {
    Expression* expr = parse_unary_expression();

    while (true) {
        if (match(TK_SLL)) {
            ShiftLeftLogical* oper = new ShiftLeftLogical(matched, expr);
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing rhs on << operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_SRL)) {
            ShiftRightLogical* oper = new ShiftRightLogical(matched, expr);
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing rhs on >>> operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else if (match(TK_SRA)) {
            ShiftRightArithmetic* oper = new ShiftRightArithmetic(matched, expr);
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing rhs on >> operator");
            } else {
                oper->set_right(expr);
            }

            expr = oper;
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_unary_expression() {
    Token oper;
    Ast* subexpr;
    Ast* expr = nullptr;

    if (lookahead(TK_LOGICAL_NOT) || lookahead(TK_NOT)) {
        expr = parse_logical_not();
    } else if (lookahead(TK_BITWISE_AND)) {
        expr = parse_address_of();
    } else if (lookahead(TK_TIMES) || lookahead(TK_POWER)) {
        expr = parse_dereference();
    } else if (lookahead(TK_BITWISE_NOT)) {
        expr = parse_bitwise_not();
    } else if (lookahead(TK_MINUS)) {
        expr = parse_unary_minus();
    } else if (lookahead(TK_PLUS)) {
        expr = parse_unary_plus();
    } else if (lookahead(TK_INC)) {
        expr = parse_pre_increment();
    } else if (lookahead(TK_DEC)) {
        expr = parse_pre_decrement();
    } else if (match(TK_DOUBLE_DOLAR)) {
        oper = matched;
       // expr = new DoubleDolar(oper, parse_unary_expression());
    } else if (lookahead(TK_SIZEOF)) {
        expr = parse_sizeof();
    } else if (lookahead(TK_NEW)) {
        //expr = parse_new_expression();
    } else if (lookahead(TK_DELETE)) {
        //expr = parse_delete_expression();
    } else {
        expr = parse_postfix_expression();
    }

    return (Expression*) expr;
}

Expression* Parser::parse_logical_not() {
    Expression* expr = nullptr;

    if (match(TK_LOGICAL_NOT) || match(TK_NOT)) {
        auto* oper = new LogicalNot(matched);

        if (!next_token_on_same_line()) {
            log_error("missing expression for 'not' operator");
        } else {
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing expression for 'not' operator");
            } else {
                oper->set_expression(expr);
            }
        }

        expr = oper;
    }

    return expr;
}

Expression* Parser::parse_address_of() {
    Expression* expr = nullptr;

    if (match(TK_BITWISE_AND)) {
        auto* oper = new AddressOf(matched);

        if (!next_token_on_same_line()) {
            log_error("missing expression for '&' operator");
        } else {
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing expression for '&' operator");
            } else {
                oper->set_expression(expr);
            }
        }

        expr = oper;
    }

    return expr;
}

Expression* Parser::parse_dereference() {
    Expression* expr = nullptr;
    Expression* subexpr = nullptr;
    Dereference* oper = nullptr;
    Token token;
    bool power = false;

    if (match(TK_TIMES)) {
        power = false;
        oper = new Dereference(matched);
        token = matched;
    } else if (match(TK_POWER)) {
        power = true;
        oper = new Dereference(matched);
        token = matched;
    } else {
        log_error("error while parsing dereference");
    }

    if (!next_token_on_same_line()) {
        log_error("expected an expression for '*' operator. Maybe you typed in another line");
    } else {
        subexpr = parse_unary_expression();

        if (subexpr == nullptr) {
            log_error("expected an expression for '*' operator");
        }

        oper->set_expression(subexpr);

        if (power) {
            oper = new Dereference(token, oper);
        }

        expr = oper;
    }

    return expr;
}

Expression* Parser::parse_bitwise_not() {
    Expression* expr = nullptr;

    if (match(TK_BITWISE_NOT)) {
        auto* oper = new BitwiseNot(matched);

        if (!next_token_on_same_line()) {
            log_error("missing expression for '~' operator");
        } else {
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing expression for '~' operator");
            } else {
                oper->set_expression(expr);
            }
        }

        expr = oper;
    }

    return expr;
}

Expression* Parser::parse_unary_minus() {
    Expression* expr = nullptr;

    if (match(TK_MINUS)) {
        auto* oper = new UnaryMinus(matched);

        if (!next_token_on_same_line()) {
            log_error("missing expression for 'unary -' operator");
        } else {
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing expression for 'unary -' operator");
            } else {
                oper->set_expression(expr);
            }
        }

        expr = oper;
    }

    return expr;
}

Expression* Parser::parse_unary_plus() {
    Expression* expr = nullptr;

    if (match(TK_PLUS)) {
        auto* oper = new UnaryPlus(matched);

        if (!next_token_on_same_line()) {
            log_error("missing expression for 'unary +' operator");
        } else {
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing expression for 'unary +' operator");
            } else {
                oper->set_expression(expr);
            }
        }

        expr = oper;
    }

    return expr;
}
Expression* Parser::parse_pre_increment() {
    Expression* expr = nullptr;

    if (match(TK_INC)) {
        auto* oper = new PreIncrement(matched);

        if (!next_token_on_same_line()) {
            log_error("missing expression for 'pre increment ++' operator");
        } else {
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing expression for 'pre increment ++' operator");
            } else {
                oper->set_expression(expr);
            }
        }

        expr = oper;
    }

    return expr;
}

Expression* Parser::parse_pre_decrement() {
    Expression* expr = nullptr;

    if (match(TK_DEC)) {
        auto* oper = new PreDecrement(matched);

        if (!next_token_on_same_line()) {
            log_error("missing expression for 'pre decrement --' operator");
        } else {
            expr = parse_unary_expression();

            if (expr == nullptr) {
                log_error("missing expression for 'pre decrement --' operator");
            } else {
                oper->set_expression(expr);
            }
        }

        expr = oper;
    }

    return expr;
}

Ast* Parser::parse_sizeof() {
    expect(TK_SIZEOF);

    Ast* expr = new Ast(AST_SIZEOF, matched);

    expect(TK_LEFT_PARENTHESIS);
    Ast* subexpr = parse_expression();

    if (subexpr) {
        expr->add_child(subexpr);
    } else {
        log_error("missing expression for sizeof operator");
    }

    expect(TK_RIGHT_PARENTHESIS);
    return expr;
}

Ast* Parser::parse_simple_unary_operator(AstKind ast_type, TokenKind token_type, const char* oper) {
    std::stringstream ss;
    Ast* expr;
    Ast* subexpr;

    expect(token_type);
    expr = new Ast(ast_type, matched);

    if (!next_token_on_same_line()) {
        ss << "expected an expression for unary '";
        ss << oper << "' operator";
        log_error(ss.str());
    } else {
        subexpr = parse_unary_expression();

        if (subexpr == nullptr) {
            ss << "expected an expression for unary '";
            ss << oper << "' operator";
            log_error(ss.str());
        }

        expr->add_child(subexpr);
    }

    return expr;
}

Ast* Parser::parse_postfix_expression() {
    Expression* left = nullptr;
    Expression* right = nullptr;
    Expression* expr = (Expression*) parse_primary_expression();

    while (true) {
        if (match(TK_DOT)) {
            left = expr;
            Dot* dot = new Dot(matched, left);
            right = parse_generic_application();

            if (right == nullptr) {
                log_error("missing member field in member access");
            } else {
                dot->set_right(right);
            }

            expr = dot;
        } else if (match(TK_ARROW)) {
            left = expr;
            Arrow* arrow = new Arrow(matched, left);
            right = parse_generic_application();

            if (right == nullptr) {
                log_error("missing member field in arrow member access");
            } else {
                arrow->set_right(right);
            }

            expr = arrow;
        } else if (match_same_line(TK_LEFT_SQUARE_BRACKET)) {
            left = expr;
            //expr = new Ast(AST_INDEX, matched);
            right = parse_expression();

            if (right == nullptr) {
                log_error("missing index expression in array access");
            } else {
                expr->add_child(left);
                expr->add_child(right);
            }

            expect(TK_RIGHT_SQUARE_BRACKET);
        } else if (match_same_line(TK_LEFT_PARENTHESIS)) {
            left = expr;
            //expr = new Ast(AST_CALL, matched);
            expr->add_child(left);
            expr->add_child(parse_argument_list());
            expect(TK_RIGHT_PARENTHESIS);
        } else if (match_same_line(TK_INC)) {
            left = expr;
           // expr = new Ast(AST_POS_INCREMENT, matched);
            expr->add_child(left);
        } else if (match_same_line(TK_DEC)) {
            left = expr;
           // expr = new Ast(AST_POS_DECREMENT, matched);
            expr->add_child(left);
        } else {
            break;
        }
    }

    return expr;
}

Expression* Parser::parse_primary_expression() {
    Expression* expr = nullptr;

    if (lookahead(TK_ID) || lookahead(TK_SCOPE)) {
        expr = parse_generic_application();
    } else if (match(TK_THIS)) {
        expr = new This(matched);
    } else if (match(TK_TRUE) || match(TK_FALSE)) {
        expr = new BooleanLiteral(matched);
    } else if (match(TK_LITERAL_INTEGER)) {
        expr = new IntegerLiteral(matched);
    } else if (match(TK_LITERAL_FLOAT)) {
        expr = new FloatLiteral(matched);
    } else if (match(TK_LITERAL_CHAR)) {
        expr = new CharLiteral(matched);
    } else if (match(TK_LITERAL_STRING)) {
        expr = new StringLiteral(matched);
    } else if (match(TK_LITERAL_SYMBOL)) {
        expr = new SymbolLiteral(matched);
    } else if (match(TK_NULL)) {
        expr = new Null(matched);
    } else if (lookahead(TK_LEFT_PARENTHESIS)) {
        expr = parse_parenthesis_or_tuple_or_sequence();
    } else if (lookahead(TK_LEFT_SQUARE_BRACKET)) {
        expr = parse_list_expression();
    } else if (lookahead(TK_LEFT_CURLY_BRACKET)) {
        expr = (Expression*) parse_array_or_hash_expression();
    } else if (lookahead(TK_BITWISE_OR)) {
        expr = (Expression*) parse_lambda();
    }

    return expr;
}

Ast* Parser::parse_delete_expression() {/*
    Token oper;
    Expression* expr = nullptr;

    expect(TK_DELETE);
    oper = matched;

    if (match(TK_LEFT_SQUARE_BRACKET)) {
        expect(TK_RIGHT_SQUARE_BRACKET);
        //expr = new DeleteArray(oper, parse_expression());
    } else {
        //expr = new Delete(oper, parse_expression());
    }

    return expr;*/
    return nullptr;
}

Expression* Parser::parse_parenthesis_or_tuple_or_sequence() {
    Token token;
    Expression* expr = nullptr;
    Expression* subexpr = nullptr;

    expect(TK_LEFT_PARENTHESIS);
    token = matched;
    subexpr = parse_expression();

    if (subexpr == nullptr) {
        log_error("expression can't be null inside parenthesis");
        return nullptr;
    }

    if (lookahead(TK_COMMA)) {
        auto oper = new Tuple();
        oper->set_token(token);
        oper->add_expression(subexpr);

        while (match(TK_COMMA)) {
            if (!lookahead(TK_RIGHT_PARENTHESIS)) {
                subexpr = parse_expression();

                if (subexpr == nullptr) {
                    log_error("missing expression inside tuple");
                } else {
                    oper->add_expression(subexpr);
                }
            }
        }

        expr = oper;
    } else if (lookahead(TK_SEMICOLON)) {
        auto oper = new Sequence();
        oper->set_token(token);
        oper->add_expression(subexpr);

        while (match(TK_SEMICOLON)) {
            if (!lookahead(TK_RIGHT_PARENTHESIS)) {
                subexpr = parse_expression();

                if (subexpr == nullptr) {
                    log_error("expression can't be null on sequence");
                } else {
                    oper->add_expression(subexpr);
                }
            }
        }

        expr = oper;
    } else {
        auto oper = new Parenthesis(token);
        oper->set_expression(subexpr);
        expr = oper;
    }

    expect(TK_RIGHT_PARENTHESIS);
    return expr;
}

Expression* Parser::parse_list_expression() {
    Token token;
    Expression* expr = nullptr;

    if (!lookahead(TK_LEFT_SQUARE_BRACKET)) {
        return nullptr;
    }

    expect(TK_LEFT_SQUARE_BRACKET);
    token = matched;

    if (!lookahead(TK_RIGHT_SQUARE_BRACKET)) {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("expected expression on list literal");
            return nullptr;
        }

        auto list = new List();
        list->set_token(token);
        list->add_expression(expr);

        while (match(TK_COMMA)) {
            if (!lookahead(TK_RIGHT_SQUARE_BRACKET)) {
                expr = parse_expression();

                if (expr == nullptr) {
                    log_error("expected expression on list literal");
                } else {
                    list->add_expression(expr);
                }
            }
        }

        expr = list;
    }

    expect(TK_RIGHT_SQUARE_BRACKET);
    return expr;
}

Ast* Parser::parse_array_or_hash_expression() {
    Ast* expr;
    Ast* array_or_hash = nullptr;

    expect(TK_LEFT_CURLY_BRACKET);

    if (!lookahead(TK_RIGHT_CURLY_BRACKET)) {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing expression on array or hash literal");
        }

        if (lookahead(TK_COLON)) {
            array_or_hash = parse_hash(expr);
        } else {
            array_or_hash = new Ast(AST_ARRAY);
            array_or_hash->add_child(expr);

            while (match(TK_COMMA)) {
                if (!lookahead(TK_RIGHT_CURLY_BRACKET)) {
                    expr = parse_expression();

                    if (expr == nullptr) {
                        log_error("missing expression on array literal");
                    }

                    array_or_hash->add_child(expr);
                }
            }
        }
    }

    expect(TK_RIGHT_CURLY_BRACKET);
    return array_or_hash;
}

Ast* Parser::parse_hash(Ast* key) {
    Ast* value = nullptr;
    Ast* pair = nullptr;
    Ast* hash = new Ast(AST_HASH);

    expect(TK_COLON);
    value = parse_expression();

    if (value == nullptr) {
        log_error("missing value on hash pair");
    }

    pair = new Ast(AST_HASH_PAIR);
    pair->add_child(key);
    pair->add_child(value);

    hash->add_child(pair);

    while (match(TK_COMMA)) {
        if (!lookahead(TK_RIGHT_CURLY_BRACKET)) {
            key = parse_expression();
            expect(TK_COLON);
            value = parse_expression();
            pair = new Ast(AST_HASH_PAIR);
            pair->add_child(key);
            pair->add_child(value);
            hash->add_child(pair);
        }
    }

    return hash;
}

Ast* Parser::parse_lambda() {
    Ast* parameters = nullptr;

    expect(TK_BITWISE_OR);
    Ast* lambda = new Ast(AST_LAMBDA);

    if (!lookahead(TK_BITWISE_OR)) {
        do {
            expect(TK_ID);
            Ast* parameter = new Ast(AST_VARIABLE, matched);

            if (match(TK_COLON)) {
                Ast* type = parse_type();

                if (type == nullptr) {
                    log_error("missing type on lambda parameter");
                } else {
                    parameter->add_child(AST_TYPE, type);
                }
            }

            if (match(TK_ASSIGNMENT)) {
                Ast* expr = parse_expression();

                if (expr == nullptr) {
                    log_error("missing expression on lambda parameter");
                } else {
                    parameter->add_child(AST_EXPRESSION, expr);
                }
            }

            lambda->add_child(parameter);
        } while (match(TK_COMMA));
    }

    expect(TK_BITWISE_OR);

    if (match(TK_ARROW)) {
        Ast* type = parse_type();

        if (type == nullptr) {
            log_error("missing return type on lambda definition");
        } else {
            lambda->add_child(AST_TYPE, type);
        }
    }

    expect(TK_LEFT_CURLY_BRACKET);
    lambda->add_child(parse_statements());
    expect(TK_RIGHT_CURLY_BRACKET);

    return lambda;
}

Ast* Parser::parse_argument_list() {
    bool has_named_arguments = false;
    Ast* expr;
    Ast* name;
    Ast* arguments = new Ast(AST_ARGUMENTS);

    if (!lookahead(TK_RIGHT_PARENTHESIS)) {
        do {
            name = nullptr;

            if (lookahead(TK_ID) && lookahead(TK_COLON, 1)) {
                expect(TK_ID);
                name = new Ast(AST_NAMED_ARGUMENT, matched);
                expect(TK_COLON);
                has_named_arguments = true;
            } else if (has_named_arguments) {
                if (lookahead(TK_ID) && lookahead(TK_COLON, 1)) {
                    expect(TK_ID);
                    name = new Ast(AST_NAMED_ARGUMENT, matched);
                    expect(TK_COLON);
                } else {
                    log_error("Missing named argument. You're using named arguments, but now is trying to type an unamed argument");
                }
            }

            expr = parse_expression();

            if (expr == nullptr) {
                log_error("Expected an expression, but got something else");
            }

            if (name) {
                name->add_child(expr);
                arguments->add_child(name);
            } else {
                arguments->add_child(expr);
            }
        } while (match(TK_COMMA));
    }

    return arguments;
}


Ast* Parser::parse_new_expression() {/*
    Ast* type;
    New* expr = new New();

    expect(TK_NEW);
    expr->set_token(matched);

    type = parse_type();

    if (type == nullptr) {
        assert(false && "missing type on new");
    }

    //expr->set_type(type);

    if (match(TK_LEFT_PARENTHESIS)) {
        //expr->set_arguments(parse_argument_list());
        expect(TK_RIGHT_PARENTHESIS);
    }

    return expr;*/
    return nullptr;
}

Expression* Parser::parse_generic_application() {
    Generics* generics;
    Expression* id = parse_scope();

    if (id == nullptr) {
        return nullptr;
    }

    generics = parse_generics();

    if (generics) {
        return new GenericsApplication(id, generics);
    }

    return id;
}

Expression* Parser::parse_scope() {
    Scope* scoped = nullptr;
    Identifier* name;
    Identifier* alias;

    if (match(TK_SCOPE)) {
        scoped = new Scope(matched);
        name = parse_identifier();

        if (name) {
            scoped->set_name(name);
        } else {
            log_error("missing identifier in global scope");
        }
    } else if (lookahead(TK_ID)) {
        alias = parse_identifier();

        if (match(TK_SCOPE)) {
            scoped = new Scope(matched);
            scoped->set_alias(alias);
            name = parse_identifier();

            if (name) {
                scoped->set_name(name);
            } else {
                log_error("missing name in scope");
            }
        } else {
            return alias;
        }
    }

    return scoped;
}

Identifier* Parser::parse_identifier() {
    Identifier* id = nullptr;

    if (match(TK_ID)) {
        id = new Identifier(matched);
    }

    return id;
}

Generics* Parser::parse_generics() {
    Type* type;
    Generics* generics = nullptr;

    if (match(TK_BEGIN_TEMPLATE)) {
        generics = new Generics();

        do {
            type = parse_type();

            if (type == nullptr) {
                log_error("missing type inside generics");
            } else {
                generics->add_type(type);
            }
        } while (match(TK_COMMA));

        expect(TK_END_TEMPLATE);
    }

    return generics;
}

void Parser::advance() {
    if (idx < tokens.size()) {
        ++idx;
    }
}

bool Parser::lookahead(int kind, int offset) {
    if (idx + offset < tokens.size()) {
        return tokens[idx + offset].get_kind() == kind;
    }

    return false;
}

bool Parser::match(int kind) {
    if (lookahead(kind)) {
        matched = tokens[idx];
        advance();
        return true;
    }

    return false;
}

bool Parser::match_same_line(int kind) {
    return next_token_on_same_line() && match(kind);
}

bool Parser::match() {
    matched = tokens[idx];
    advance();

    return true;
}

bool Parser::expect(int kind) {
    std::stringstream ss;

    if (match(kind)) {
        return true;
    }

    match();

    ss << "Expected a " << token_kind_to_str_map.at(kind)
              << " but got a " << token_kind_to_str_map.at(matched.get_kind());

    log_error(ss.str());
    return false;
}

bool Parser::has_next(int offset) {
    return idx + offset < tokens.size();
}

void Parser::recover() {
    bool flag = true;

    while (flag) {
        flag = false;
        flag = flag || lookahead(TK_DEF);
        flag = flag || lookahead(TK_CLASS);
        flag = flag || lookahead(TK_UNION);
        flag = flag || lookahead(TK_STRUCT);
        flag = flag || lookahead(TK_ENUM);
        flag = flag || lookahead(TK_EOF);
        advance();
    }
}

void Parser::unexpected_token() {
    match();
    log_error("unexpected token");
}

void Parser::indent() {
    indent_stack.push(matched.get_whitespace());
}

void Parser::dedent() {
    indent_stack.pop();
}

bool Parser::is_indented() {
    if (has_next()) {
        return tokens[idx].get_whitespace() > indent_stack.top();
    }

    return false;
}

bool Parser::has_parameters() {
    return lookahead(TK_AT) && lookahead(TK_ID, 1) && lookahead(TK_COLON, 2);
}

bool Parser::next_token_on_same_line() {
    if (has_next() && !lookahead(TK_EOF)) {
        return matched.get_line() == tokens[idx].get_line();
    }

    return false;
}

