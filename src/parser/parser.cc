#include <iostream>
#include <sstream>
#include <cassert>

#include "parser/parser.h"
#include "scanner/scanner.h"
#include "log/logs.h"

using namespace haard;

Parser::Parser() {
    idx = 0;
}

Ast* Parser::read(std::string path, std::string relative_path) {
    Ast* module;
    Scanner sc;

    idx = 0;
    tokens = sc.read(path);
    module = parse_module();

    return module;
}

Ast* Parser::parse_module() {
    Ast* module = new Ast(AST_MODULE);

    while (true) {
        if (lookahead(TK_IMPORT)) {
            module->add_child(parse_import());
        } else if (lookahead(TK_DEF)) {
            module->add_child(parse_function());
        } else if (lookahead(TK_CLASS)) {
        //    module->add_class(parse_class());
        } else if (lookahead(TK_STRUCT)) {
          //  module->add_struct(parse_struct());
        } else if (lookahead(TK_UNION)) {
            //module->add_union(parse_union());
        } else if (lookahead(TK_ENUM)) {
            //module->add_enum(parse_enum());
        } else {
            break;
        }
    }

    return module;
}

Ast* Parser::parse_import() {
    Ast* import = new Ast(AST_IMPORT);
    Ast* path = new Ast(AST_IMPORT_PATH);
    Ast* alias = nullptr;

    expect(TK_IMPORT);
    import->set_from_token(matched);

    do {
        expect(TK_ID);
        path->add_child(AST_IMPORT_PATH_MEMBER, matched);
    } while (match(TK_DOT));

    if (match(TK_AS)) {
        expect(TK_ID);
        alias = new Ast(AST_IMPORT_ALIAS, matched);
    }

    import->add_child(path);
    import->add_child(alias);
    return import;
}

Ast* Parser::parse_class() {/*
    Class* klass = new Class();

    expect(TK_CLASS);
    expect(TK_ID);
    klass->set_name(matched);
    klass->set_generics(parse_generics());

    if (match(TK_LEFT_PARENTHESIS)) {
        klass->set_super_type(parse_type());
        expect(TK_RIGHT_PARENTHESIS);
    }

    expect(TK_COLON);
    indent();

    while (is_indented()) {
        if (lookahead(TK_DEF)) {
            klass->add_function(parse_function());
        } else if (lookahead(TK_ID)) {
            klass->add_variable(parse_variable());
        } else if (match(TK_PASS)) {
            break;
        } else {
            break;
        }
    }

    dedent();
    return klass;*/
}

Ast* Parser::parse_struct() {/*
    Struct* st = new Struct();

    expect(TK_STRUCT);
    expect(TK_ID);
    st->set_name(matched);
    st->set_generics(parse_generics());

    if (match(TK_LEFT_PARENTHESIS)) {
        st->set_super_type(parse_type());
        expect(TK_RIGHT_PARENTHESIS);
    }

    expect(TK_COLON);
    indent();

    while (is_indented()) {
        if (lookahead(TK_DEF)) {
            st->add_function(parse_function());
        } else if (lookahead(TK_ID)) {
            st->add_variable(parse_variable());
        } else if (match(TK_PASS)) {
            break;
        } else {
            break;
        }
    }

    dedent();
    return st;*/
}

Ast* Parser::parse_union() {/*
    Union* st = new Union();

    expect(TK_UNION);
    expect(TK_ID);
    st->set_name(matched);
    st->set_generics(parse_generics());

    if (match(TK_LEFT_PARENTHESIS)) {
        st->set_super_type(parse_type());
        expect(TK_RIGHT_PARENTHESIS);
    }

    expect(TK_COLON);
    indent();

    while (is_indented()) {
        if (lookahead(TK_DEF)) {
            st->add_function(parse_function());
        } else if (lookahead(TK_ID)) {
            st->add_variable(parse_variable());
        } else if (match(TK_PASS)) {
            break;
        } else {
            break;
        }
    }

    dedent();
    return st;*/
}

Ast* Parser::parse_enum() {/*
    Enum* st = new Enum();

    expect(TK_ENUM);
    expect(TK_ID);
    st->set_name(matched);
    st->set_generics(parse_generics());

    if (match(TK_LEFT_PARENTHESIS)) {
        st->set_super_type(parse_type());
        expect(TK_RIGHT_PARENTHESIS);
    }

    expect(TK_COLON);
    indent();

    while (is_indented()) {
        if (lookahead(TK_DEF)) {
            st->add_function(parse_function());
        } else if (lookahead(TK_ID)) {
            st->add_variable(parse_enum_variable());
        } else if (match(TK_PASS)) {
            break;
        } else {
            break;
        }
    }

    dedent();
    return st;*/
}

Ast* Parser::parse_variable() {/*
    Variable* var = new Variable();

    expect(TK_ID);
    var->set_name(matched);

    expect(TK_COLON);
    var->set_type(parse_type());

    if (match(TK_ASSIGNMENT)) {
        Expression* expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing expression on member");
        } else {
            var->set_expression(expr);
        }
    }

    return var;*/
}

Ast* Parser::parse_enum_variable() {/*
    Variable* var = new Variable();

    expect(TK_ID);
    var->set_name(matched);

    if (match(TK_COLON)) {
        var->set_type(parse_type());
    }

    if (match(TK_ASSIGNMENT)) {
        Expression* expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing expression on enum member");
        } else {
            var->set_expression(expr);
        }
    }

    return var;*/
}

Ast* Parser::parse_function() {
    Ast* function = new Ast(AST_FUNCTION);

    expect(TK_DEF);
    expect(TK_ID);
    function->set_from_token(matched);
    function->add_child(parse_generics());

    expect(TK_COLON);
    indent();

    Ast* return_type = parse_type();

    if (return_type) {
        function->add_child(AST_RETURN_TYPE, return_type);
    } else {
        log_error("Expected return type in function " + function->get_value());
    }

    function->add_child(parse_parameters());
    function->add_child(parse_statements());

    dedent();

    return function;
}

Ast* Parser::parse_parameters() {
    if (!has_parameters()) {
        return nullptr;
    }

    Ast* parameters = new Ast(AST_PARAMETERS);

    while (has_parameters()) {
        parameters->add_child(parse_parameter());
    }

    return parameters;
}

Ast* Parser::parse_parameter() {
    Ast* param = new Ast(AST_PARAMETER);

    expect(TK_AT);
    expect(TK_ID);
    param->set_from_token(matched);

    expect(TK_COLON);
    Ast* type = parse_type();

    if (type != nullptr) {
        param->add_child(type);
    } else {
        log_error("expected type in parameter");
    }

    if (match(TK_ASSIGNMENT)) {
        Ast* expr = parse_expression();

        if (expr == nullptr) {
            log_error("missing expression on default parameter value");
        }

        param->add_child(expr);
    }

    return param;
}

Ast* Parser::parse_statement() {
    Ast* stmt = nullptr;
    Ast* expr = nullptr;

    if (lookahead(TK_WHILE)) {
        stmt = parse_while_statement();
    } else if (lookahead(TK_FOR)) {
        stmt = parse_for_statement();
    } else if (lookahead(TK_IF)) {
        stmt = parse_if_statement();
    } else if (lookahead(TK_RETURN)) {
        stmt = parse_return_statement();
    } else if (lookahead(TK_SWITCH)) {
        stmt = parse_switch_statement();
    } else {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("expected an expression statement");
            //recover();
            unexpected_token();
        }

        stmt = new Ast(AST_EXPRESSION);
        stmt->add_child(expr);

        if (match(TK_SEMICOLON)) {
            stmt->set_kind(AST_EXPRESSION_WITH_SEMICOLON);
        }
    }

    return stmt;
}

Ast* Parser::parse_while_statement() {
    Ast* condition;
    Ast* stmt = new Ast(AST_WHILE);

    expect(TK_WHILE);
    stmt->set_from_token(matched);
    condition = parse_expression();

    if (condition == nullptr) {
        log_error("expected while condition");
    }

    stmt->add_child(condition);
    expect(TK_COLON);
    indent();
    stmt->add_child(parse_statements());
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
}


Ast* Parser::parse_return_statement() {
    Ast* stmt = new Ast(AST_RETURN);
    Ast* expr = nullptr;

    expect(TK_RETURN);

    if (next_token_on_same_line()) {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("expected an expression on return statement but got an invalid token");
        }

        stmt->add_child(expr);
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

Ast* Parser::parse_statements() {
    Ast* statements = new Ast(AST_COMPOUND_STATEMENT);

    if (match(TK_PASS)) {
        return statements;
    }

    while (is_indented() && !lookahead(TK_RIGHT_CURLY_BRACKET)) {
        statements->add_child(parse_statement());
    }

    return statements;
}

Ast* Parser::parse_type() {
    return parse_tuple_or_function_type();
}

Ast* Parser::parse_tuple_or_function_type() {
    Ast* type;
    Ast* types;

    if (!lookahead(TK_LEFT_PARENTHESIS)) {
        return parse_primary_type();
    }

    expect(TK_LEFT_PARENTHESIS);
    types = parse_type_list(AST_TYPE_TUPLE);
    expect(TK_RIGHT_PARENTHESIS);

    if (match(TK_ARROW)) {
        if (!next_token_on_same_line()) {
            assert(false && "return type should be on same line");
        }

        Ast* return_type = parse_type();

        if (return_type == nullptr) {
            assert(false && "expected return type");
        }

        type = new Ast(AST_TYPE_FUNCTION);
        type->add_child(types);
        type->add_child(return_type);
    } else {
        type = types;
    }

    return type;
}

Ast* Parser::parse_primary_type() {
    Ast* type = nullptr;
    Ast* subtype = nullptr;

    if (match(TK_INT)) {
       type = new Ast(AST_TYPE_INT, matched);
    } else if (match(TK_UINT)) {
       type = new Ast(AST_TYPE_UINT, matched);
    } else if (match(TK_FLOAT)) {
       type = new Ast(AST_TYPE_FLOAT, matched);
    } else if (match(TK_DOUBLE)) {
       type = new Ast(AST_TYPE_DOUBLE, matched);
    } else if (match(TK_SHORT)) {
       type = new Ast(AST_TYPE_SHORT, matched);
    } else if (match(TK_USHORT)) {
       type = new Ast(AST_TYPE_USHORT, matched);
    } else if (match(TK_LONG)) {
       type = new Ast(AST_TYPE_LONG, matched);
    } else if (match(TK_ULONG)) {
       type = new Ast(AST_TYPE_ULONG, matched);
    } else if (match(TK_CHAR)) {
       type = new Ast(AST_TYPE_CHAR, matched);
    } else if (match(TK_UCHAR)) {
       type = new Ast(AST_TYPE_UCHAR, matched);
    } else if (match(TK_SYMBOL)) {
       type = new Ast(AST_TYPE_SYMBOL, matched);
    } else if (match(TK_VOID)) {
       type = new Ast(AST_TYPE_VOID, matched);
    } else if (match(TK_BOOL)) {
       type = new Ast(AST_TYPE_BOOL, matched);
    } else if (match(TK_STR)) {
       type = new Ast(AST_TYPE_STR, matched);
    } else if (match(TK_I8)) {
       type = new Ast(AST_TYPE_I8, matched);
    } else if (match(TK_I16)) {
       type = new Ast(AST_TYPE_I16, matched);
    } else if (match(TK_I32)) {
       type = new Ast(AST_TYPE_I32, matched);
    } else if (match(TK_I64)) {
       type = new Ast(AST_TYPE_I64, matched);
    } else if (match(TK_U8)) {
       type = new Ast(AST_TYPE_U8, matched);
    } else if (match(TK_U16)) {
       type = new Ast(AST_TYPE_U16, matched);
    } else if (match(TK_U32)) {
       type = new Ast(AST_TYPE_U32, matched);
    } else if (match(TK_U64)) {
       type = new Ast(AST_TYPE_U64, matched);
    } else if (match(TK_F32)) {
       type = new Ast(AST_TYPE_F32, matched);
    } else if (match(TK_F64)) {
       type = new Ast(AST_TYPE_F64, matched);
    } else if (match(TK_LEFT_SQUARE_BRACKET)) {
        subtype = parse_type();

        if (subtype == nullptr) {
            assert(false && "no type");
        } else {
            type = new Ast(AST_TYPE_LIST);
            type->add_child(subtype);
        }

        expect(TK_RIGHT_SQUARE_BRACKET);
    } else if (lookahead(TK_ID) || lookahead(TK_SCOPE)) {
        Ast* id = parse_generic_instantiation();
        type = new Ast(AST_TYPE_NAMED);
        type->add_child(id);
    }

    while (type != nullptr) {
        if (match_same_line(TK_TIMES)) {
            subtype = type;
            type = new Ast(AST_TYPE_POINTER, matched);
            type->add_child(subtype);
        } else if (match_same_line(TK_POWER)) {
            subtype = type;
            type = new Ast(AST_TYPE_POINTER, matched);
            type->add_child(subtype);

            subtype = type;
            type = new Ast(AST_TYPE_POINTER, matched);
            type->add_child(subtype);
        } else if (match_same_line(TK_BITWISE_AND)) {
            subtype = type;
            type = new Ast(AST_TYPE_REFERENCE, matched);
            type->add_child(subtype);
        } else if (match_same_line(TK_LOGICAL_AND)) {
            subtype = type;
            type = new Ast(AST_TYPE_REFERENCE, matched);
            type->add_child(subtype);

            subtype = type;
            type = new Ast(AST_TYPE_REFERENCE, matched);
            type->add_child(subtype);
        } else if (match_same_line(TK_LEFT_SQUARE_BRACKET)) {
            subtype = type;
            type = new Ast(AST_TYPE_ARRAY, matched);
            type->add_child(subtype);
            //type->add_child(parse_expression());
            expect(TK_RIGHT_SQUARE_BRACKET);
        } else {
            break;
        }
    }

    return type;
}

Ast* Parser::parse_expression() {
    return parse_assignment_expression();
}

Ast* Parser::parse_assignment_expression() {
    Ast* expr = parse_cast_expression();

    while (true) {
        if (match(TK_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_ASSIGNMENT, "=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_BITWISE_AND_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_BITWISE_AND_ASSIGNMENT, "&=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_BITWISE_XOR_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_BITWISE_XOR_ASSIGNMENT, "^=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_BITWISE_OR_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_BITWISE_OR_ASSIGNMENT, "|=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_BITWISE_NOT_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_BITWISE_NOT_ASSIGNMENT, "~=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_DIVISION_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_DIVISION_ASSIGNMENT, "/=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_INTEGER_DIVISION_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_INTEGER_DIVISION_ASSIGNMENT, "//=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_MINUS_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_MINUS_ASSIGNMENT, "-=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_MODULO_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_MODULO_ASSIGNMENT, "%=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_PLUS_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_PLUS_ASSIGNMENT, "+=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_TIMES_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_TIMES_ASSIGNMENT, "*=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_SLL_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_SHIFT_LEFT_LOGICAL_ASSIGNMENT, "<<=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_SRA_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT, ">>=", expr, &Parser::parse_cast_expression);
        } else if (match(TK_SRL_ASSIGNMENT)) {
            expr = parse_binary_operator(AST_SHIFT_RIGHT_LOGICAL_ASSIGNMENT, ">>>=", expr, &Parser::parse_cast_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_cast_expression() {
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

    return expr;
}

Ast* Parser::parse_logical_or_expression() {
    Ast* expr = parse_logical_and_expression();

    while (true) {
        if (match(TK_LOGICAL_OR)) {
            expr = parse_binary_operator(AST_LOGICAL_OR, "||", expr, &Parser::parse_logical_and_expression);
        } else if (match(TK_OR)) {
            expr = parse_binary_operator(AST_OR, "or", expr, &Parser::parse_logical_and_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_logical_and_expression() {
    Ast* expr = parse_equality_expression();

    while (true) {
        if (match(TK_AND) || match(TK_LOGICAL_AND)) {
            expr = parse_binary_operator(AST_LOGICAL_AND, "&&", expr, &Parser::parse_equality_expression);
        } else if (match(TK_AND)) {
            expr = parse_binary_operator(AST_AND, "and", expr, &Parser::parse_equality_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_equality_expression() {
    Ast* expr = parse_relational_expression();

    while (true) {
        if (match(TK_EQ)) {
            expr = parse_binary_operator(AST_EQUAL, "==", expr, &Parser::parse_relational_expression);
        } else if (match(TK_NE)) {
            expr = parse_binary_operator(AST_NOT_EQUAL, "!=", expr, &Parser::parse_relational_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_relational_expression() {
    Ast* expr = parse_range_expression();

    while (true) {
        if (match(TK_LT)) {
            expr = parse_binary_operator(AST_LESS_THAN, "<", expr, &Parser::parse_range_expression);
        } else if (match(TK_GT)) {
            expr = parse_binary_operator(AST_GREATER_THAN, ">", expr, &Parser::parse_range_expression);
        } else if (match(TK_LE)) {
            expr = parse_binary_operator(AST_LESS_THAN_OR_EQUAL, "<=", expr, &Parser::parse_range_expression);
        } else if (match(TK_GE)) {
            expr = parse_binary_operator(AST_GREATER_THAN_OR_EQUAL, ">=", expr, &Parser::parse_range_expression);
        } else if (match(TK_IN)) {
            expr = parse_binary_operator(AST_IN, "in", expr, &Parser::parse_range_expression);
        } else if (match(TK_NOT)) {
            expect(TK_IN);
            expr = parse_binary_operator(AST_NOT_IN, "not in", expr, &Parser::parse_range_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_range_expression() {
    Ast* expr = parse_arith_expression();

    while (true) {
        if (match(TK_INCLUSIVE_RANGE)) {
            expr = parse_binary_operator(AST_INCLUSIVE_RANGE, "..", expr, &Parser::parse_arith_expression);
        } else if (match(TK_EXCLUSIVE_RANGE)) {
            expr = parse_binary_operator(AST_EXCLUSIVE_RANGE, "...", expr, &Parser::parse_arith_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_arith_expression() {
    Ast* expr = parse_term_expression();

    while (true) {
        if (match_same_line(TK_PLUS)) {
            expr = parse_binary_operator(AST_PLUS, "+", expr, &Parser::parse_term_expression);
        } else if (match_same_line(TK_MINUS)) {
            expr = parse_binary_operator(AST_MINUS, "-", expr, &Parser::parse_term_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_term_expression() {
    Ast* expr = parse_power_expression();

    while (true) {
        if (match_same_line(TK_TIMES)) {
            expr = parse_binary_operator(AST_TIMES, "*", expr, &Parser::parse_power_expression);
        } else if (match_same_line(TK_DIVISION)) {
            expr = parse_binary_operator(AST_DIVISION, "/", expr, &Parser::parse_power_expression);
        } else if (match_same_line(TK_MODULO)) {
            expr = parse_binary_operator(AST_MODULO, "%", expr, &Parser::parse_power_expression);
        } else if (match_same_line(TK_INTEGER_DIVISION)) {
            expr = parse_binary_operator(AST_INTEGER_DIVISION, "//", expr, &Parser::parse_power_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_power_expression() {
    Ast* expr = parse_bitwise_or_expression();

    if (match(TK_POWER)) {
        expr = parse_binary_operator(AST_POWER, "**", expr, &Parser::parse_power_expression);
    }

    return expr;
}

Ast* Parser::parse_bitwise_or_expression() {
    Ast* expr = parse_bitwise_xor_expression();

    while (true) {
        if (match(TK_BITWISE_OR)) {
            expr = parse_binary_operator(AST_BITWISE_OR, "|", expr, &Parser::parse_bitwise_xor_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_bitwise_xor_expression() {
    Token oper;
    Ast* expr = parse_bitwise_and_expression();

    while (true) {
        if (match(TK_BITWISE_XOR)) {
            expr = parse_binary_operator(AST_BITWISE_XOR, "^", expr, &Parser::parse_bitwise_and_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_bitwise_and_expression() {
    Token oper;
    Ast* expr = parse_shift_expression();

    while (true) {
        if (match_same_line(TK_BITWISE_AND)) {
            expr = parse_binary_operator(AST_BITWISE_AND, "&", expr, &Parser::parse_shift_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_shift_expression() {
    Ast* expr = parse_unary_expression();
    Ast* left;
    Ast* right;

    while (true) {
        if (match(TK_SLL)) {
            expr = parse_binary_operator(AST_SHIFT_LEFT_LOGICAL, "sll", expr, &Parser::parse_unary_expression);
        } else if (match(TK_SRL)) {
            expr = parse_binary_operator(AST_SHIFT_RIGHT_LOGICAL, "srl", expr, &Parser::parse_unary_expression);
        } else if (match(TK_SRA)) {
            expr = parse_binary_operator(AST_SHIFT_RIGHT_ARITHMETIC, "sra", expr, &Parser::parse_unary_expression);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_unary_expression() {
    Token oper;
    Ast* subexpr;
    Ast* expr = nullptr;

    if (lookahead(TK_LOGICAL_NOT)) {
        expr = parse_logical_not();
    } else if (lookahead(TK_NOT)) {
        expr = parse_not();
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

    return expr;
}

Ast* Parser::parse_logical_not() {
    return parse_simple_unary_operator(AST_LOGICAL_NOT, TK_LOGICAL_NOT, "!");
}

Ast* Parser::parse_not() {
    return parse_simple_unary_operator(AST_NOT, TK_NOT, "not");
}

Ast* Parser::parse_address_of() {
    return parse_simple_unary_operator(AST_ADDRESS_OF, TK_BITWISE_AND, "&");
}

Ast* Parser::parse_dereference() {
    Ast* expr = new Ast(AST_DEREFERENCE, matched);
    Ast* subexpr;
    bool power = false;

    if (match(TK_TIMES)) {
        power = false;
    } else if (match(TK_POWER)) {
        power = true;
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

        expr->add_child(subexpr);

        if (power) {
            subexpr = expr;
            expr = new Ast(AST_DEREFERENCE, matched);
            expr->add_child(subexpr);
        }
    }

    return expr;
}

Ast* Parser::parse_bitwise_not() {
    return parse_simple_unary_operator(AST_BITWISE_NOT, TK_BITWISE_NOT, "~");
}

Ast* Parser::parse_unary_minus() {
    return parse_simple_unary_operator(AST_UNARY_MINUS, TK_MINUS, "-");
}

Ast* Parser::parse_unary_plus() {
    return parse_simple_unary_operator(AST_UNARY_PLUS, TK_PLUS, "+");
}

Ast* Parser::parse_pre_increment() {
    return parse_simple_unary_operator(AST_PRE_INCREMENT, TK_INC, "++");
}

Ast* Parser::parse_pre_decrement() {
    return parse_simple_unary_operator(AST_PRE_DECREMENT, TK_DEC, "--");
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

Ast* Parser::parse_binary_operator(AstKind kind, const char* oper, Ast* left, Ast* (Parser::*function)()) {
    std::stringstream ss;
    Ast* expr = new Ast(kind, matched);
    Ast* right = (this->*function)();

    if (right == nullptr) {
        ss << "expected an rhs for operator <white>";
        ss << oper;
        ss << "<normal>";
        log_error(ss.str());
    }

    expr->add_child(left);
    expr->add_child(right);
    return expr;
}

Ast* Parser::parse_postfix_expression() {
    Ast* left = nullptr;
    Ast* right = nullptr;
    Ast* expr = parse_primary_expression();

    while (true) {
        if (match(TK_DOT)) {
            left = expr;
            expr = new Ast(AST_DOT, matched);
            right = parse_generic_instantiation();

            if (right == nullptr) {
                log_error("missing member field in member access");
            } else {
                expr->add_child(left);
                expr->add_child(right);
            }
        } else if (match(TK_ARROW)) {
            left = expr;
            expr = new Ast(AST_ARROW, matched);
            right = parse_generic_instantiation();

            if (right == nullptr) {
                log_error("missing member field in arrow member access");
            } else {
                expr->add_child(left);
                expr->add_child(right);
            }
        } else if (match_same_line(TK_LEFT_SQUARE_BRACKET)) {
            left = expr;
            expr = new Ast(AST_INDEX, matched);
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
            expr = new Ast(AST_CALL, matched);
            expr->add_child(left);
            expr->add_child(parse_argument_list());
            expect(TK_RIGHT_PARENTHESIS);
        } else if (match_same_line(TK_INC)) {
            left = expr;
            expr = new Ast(AST_POS_INCREMENT, matched);
            expr->add_child(left);
        } else if (match_same_line(TK_DEC)) {
            left = expr;
            expr = new Ast(AST_POS_DECREMENT, matched);
            expr->add_child(left);
        } else {
            break;
        }
    }

    return expr;
}

Ast* Parser::parse_primary_expression() {
    Ast* expr = nullptr;

    if (lookahead(TK_ID) || lookahead(TK_SCOPE)) {
        expr = parse_generic_instantiation();
    } else if (match(TK_THIS)) {
        expr = new Ast(AST_THIS, matched);
    } else if (match(TK_TRUE) || match(TK_FALSE)) {
        expr = new Ast(AST_LITERAL_BOOLEAN, matched);
    } else if (match(TK_LITERAL_INTEGER)) {
        expr = new Ast(AST_LITERAL_INTEGER, matched);
    } else if (match(TK_LITERAL_FLOAT)) {
        expr = new Ast(AST_LITERAL_FLOAT, matched);
    } else if (match(TK_LITERAL_DOUBLE)) {
        expr = new Ast(AST_LITERAL_DOUBLE, matched);
    } else if (match(TK_LITERAL_CHAR)) {
        expr = new Ast(AST_LITERAL_CHAR, matched);
    } else if (match(TK_LITERAL_SINGLE_QUOTE_STRING)) {
        expr = new Ast(AST_LITERAL_STRING, matched);
    } else if (match(TK_LITERAL_DOUBLE_QUOTE_STRING)) {
        expr = new Ast(AST_LITERAL_STRING, matched);
    } else if (match(TK_NULL)) {
        expr = new Ast(AST_NULL, matched);
    } else if (lookahead(TK_LEFT_PARENTHESIS)) {
        expr = parse_parenthesis_or_tuple_or_sequence();
    } else if (lookahead(TK_LEFT_SQUARE_BRACKET)) {
        expr = parse_list_expression();
    } else if (lookahead(TK_LEFT_CURLY_BRACKET)) {
        expr = parse_array_or_hash_expression();
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
}

Ast* Parser::parse_parenthesis_or_tuple_or_sequence() {
    Token oper;
    Ast* expr = nullptr;
    Ast* subexpr = nullptr;

    expect(TK_LEFT_PARENTHESIS);
    subexpr = parse_expression();

    if (subexpr == nullptr) {
        log_error("expression can't be null inside parenthesis");
        return nullptr;
    }

    expr = new Ast(AST_PARENTHESIS);
    expr->add_child(subexpr);

    if (lookahead(TK_COMMA)) {
        expr->set_kind(AST_TUPLE);

        while (match(TK_COMMA)) {
            if (!lookahead(TK_RIGHT_PARENTHESIS)) {
                subexpr = parse_expression();

                if (subexpr == nullptr) {
                    log_error("missing expression inside tuple");
                }

                expr->add_child(subexpr);
            }
        }
    } else if (lookahead(TK_SEMICOLON)) {
        expr->set_kind(AST_SEQUENCE);

        while (match(TK_SEMICOLON)) {
            if (!lookahead(TK_RIGHT_PARENTHESIS)) {
                subexpr = parse_expression();

                if (subexpr == nullptr) {
                    log_error("expression can't be null on sequence");
                }

                expr->add_child(subexpr);
            }
        }
    }

    expect(TK_RIGHT_PARENTHESIS);
    return expr;
}

Ast* Parser::parse_list_expression() {
    Ast* expr = nullptr;
    Ast* list = new Ast(AST_LIST);

    expect(TK_LEFT_SQUARE_BRACKET);

    if (!lookahead(TK_RIGHT_SQUARE_BRACKET)) {
        expr = parse_expression();

        if (expr == nullptr) {
            log_error("expected expression on list literal");
        }

        list->add_child(expr);

        while (match(TK_COMMA)) {
            if (!lookahead(TK_RIGHT_SQUARE_BRACKET)) {
                expr = parse_expression();

                if (expr == nullptr) {
                    log_error("expected expression on list literal");
                }

                list->add_child(expr);
            }
        }
    }

    expect(TK_RIGHT_SQUARE_BRACKET);
    return list;
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

Ast* Parser::parse_generic_instantiation() {
    Ast* apply = nullptr;
    Ast* generics;
    Ast* id = parse_scope();

    if (id == nullptr) {
        return nullptr;
    }

    generics = parse_generics();

    if (generics) {
        apply = new Ast(AST_GENERIC_APPLICATION);
        apply->add_child(id);
        apply->add_child(generics);
    } else {
        apply = id;
    }

    return apply;
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
}

Ast* Parser::parse_scope() {
    Ast* scoped = nullptr;
    Ast* id;
    Ast* alias;

    if (match(TK_SCOPE)) {
        scoped = new Ast(AST_SCOPE, matched);
        id = parse_identifier();

        if (id) {
            scoped->add_child(parse_identifier());
        } else {
            log_error("missing identifier in global scope");
        }
    } else if (lookahead(TK_ID)) {
        Ast* id = parse_identifier();

        if (match(TK_SCOPE)) {
            scoped = new Ast(AST_SCOPE, matched);
            scoped->add_child(id);
            alias = parse_identifier();

            if (alias) {
                scoped->add_child(alias);
            } else {
                log_error("missing name in scope");
            }
        } else {
            scoped = id;
        }
    }

    return scoped;
}

Ast* Parser::parse_identifier() {
    Ast* id = nullptr;

    if (match(TK_ID)) {
        id = new Ast(AST_ID, matched);
    }

    return id;
}

Ast* Parser::parse_generics() {
    Ast* generics = nullptr;

    if (match(TK_BEGIN_TEMPLATE)) {
        generics = parse_type_list(AST_GENERICS);
        expect(TK_END_TEMPLATE);
    }

    return generics;
}

Ast* Parser::parse_type_list(AstKind kind) {
    Ast* type_list = new Ast(kind);
    Ast* type = parse_type();

    if (type == nullptr) {
        assert(false && "type can't be null on function generics");
    }

    type_list->add_child(type);

    while (match(TK_COMMA)) {
        type = parse_type();

        if (type == nullptr) {
            assert(false && "type can't be null on function generics");
        }

        type_list->add_child(type);
    }

    return type_list;
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

