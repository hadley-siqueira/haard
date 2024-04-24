cmake_minimum_required(VERSION 3.0)
project(hdc)

include_directories(src/include)

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -g")

set(STRING_POOL_SRC
    src/string_pool/string_pool.cc
)

set(TYPE_POOL_SRC
    src/type_pool/type_pool.cc
)

set(TOKEN_SRC
    src/token/token.cc
)

set(AST_UNOP_SRC
    src/ast/unary_operators/unary_operator.cc
    src/ast/unary_operators/unary_minus.cc
    src/ast/unary_operators/unary_plus.cc
    src/ast/unary_operators/address_of.cc
    src/ast/unary_operators/dereference.cc
    src/ast/unary_operators/logical_not.cc
    src/ast/unary_operators/bitwise_not.cc
    src/ast/unary_operators/pre_increment.cc
    src/ast/unary_operators/pre_decrement.cc
    src/ast/unary_operators/pos_increment.cc
    src/ast/unary_operators/pos_decrement.cc
    src/ast/unary_operators/sizeof.cc
    src/ast/unary_operators/new.cc
    src/ast/unary_operators/delete.cc
    src/ast/unary_operators/delete_array.cc
    src/ast/unary_operators/parenthesis.cc
)

set(AST_BINOPS_SRC
    src/ast/binary_operators/binary_operator.cc

    src/ast/binary_operators/assignment.cc
    src/ast/binary_operators/bitwise_and_assignment.cc
    src/ast/binary_operators/bitwise_xor_assignment.cc
    src/ast/binary_operators/bitwise_or_assignment.cc
    src/ast/binary_operators/bitwise_not_assignment.cc
    src/ast/binary_operators/division_assignment.cc
    src/ast/binary_operators/integer_division_assignment.cc
    src/ast/binary_operators/minus_assignment.cc
    src/ast/binary_operators/modulo_assignment.cc
    src/ast/binary_operators/plus_assignment.cc
    src/ast/binary_operators/times_assignment.cc
    src/ast/binary_operators/shift_left_logical_assignment.cc
    src/ast/binary_operators/shift_right_arithmetic_assignment.cc
    src/ast/binary_operators/shift_right_logical_assignment.cc

    src/ast/binary_operators/cast.cc

    src/ast/binary_operators/logical_or.cc
    src/ast/binary_operators/logical_and.cc

    src/ast/binary_operators/equal.cc
    src/ast/binary_operators/not_equal.cc

    src/ast/binary_operators/less_than.cc
    src/ast/binary_operators/greater_than.cc
    src/ast/binary_operators/less_than_or_equal.cc
    src/ast/binary_operators/greater_than_or_equal.cc

    src/ast/binary_operators/in.cc
    src/ast/binary_operators/not_in.cc

    src/ast/binary_operators/inclusive_range.cc
    src/ast/binary_operators/exclusive_range.cc

    src/ast/binary_operators/plus.cc
    src/ast/binary_operators/minus.cc

    src/ast/binary_operators/times.cc
    src/ast/binary_operators/division.cc
    src/ast/binary_operators/modulo.cc
    src/ast/binary_operators/integer_division.cc

    src/ast/binary_operators/power.cc

    src/ast/binary_operators/bitwise_or.cc
    src/ast/binary_operators/bitwise_xor.cc
    src/ast/binary_operators/bitwise_and.cc

    src/ast/binary_operators/shift_left_logical.cc
    src/ast/binary_operators/shift_right_arithmetic.cc
    src/ast/binary_operators/shift_right_logical.cc

    src/ast/binary_operators/dot.cc
    src/ast/binary_operators/arrow.cc
    src/ast/binary_operators/index.cc
    src/ast/binary_operators/call.cc
    src/ast/binary_operators/hash_pair.cc
)

set(AST_LITERALS
    src/ast/literals/literal.cc
    src/ast/literals/boolean_literal.cc
    src/ast/literals/integer_literal.cc
    src/ast/literals/float_literal.cc
    src/ast/literals/double_literal.cc
    src/ast/literals/char_literal.cc
    src/ast/literals/string_literal.cc
    src/ast/literals/list_literal.cc
    src/ast/literals/array_literal.cc
    src/ast/literals/hash_literal.cc
    src/ast/literals/tuple_literal.cc
)

set(AST_STMTS
    src/ast/statements/while_statement.cc
    src/ast/statements/for_statement.cc
    src/ast/statements/return_statement.cc
)

set(AST_SRC
    src/ast/ast_node.cc
    src/ast/declaration.cc
    src/ast/modules.cc
    src/ast/module.cc
    src/ast/import.cc
    src/ast/function.cc
    src/ast/named_type_descriptor.cc
    src/ast/class.cc
    src/ast/struct.cc
    src/ast/union.cc
    src/ast/enum.cc
    src/ast/variable.cc
    src/ast/type.cc
    src/ast/function_type.cc
    src/ast/pointer_type.cc
    src/ast/reference_type.cc
    src/ast/list_type.cc
    src/ast/tuple_type.cc
    src/ast/array_type.cc
    src/ast/subtyped_type.cc
    src/ast/named_type.cc
    src/ast/type_list.cc
    src/ast/statement.cc
    src/ast/compound_statement.cc
    src/ast/expression_statement.cc
    src/ast/expression.cc
    src/ast/expression_list.cc
    src/ast/sequence.cc

    ${AST_UNOP_SRC}
    ${AST_BINOPS_SRC}
    ${AST_LITERALS}
    ${AST_STMTS}

    src/ast/identifier.cc
)

set(SCANNER_SRC
    src/scanner/scanner.cc
)

set(PARSER_SRC
    src/parser/parser.cc
)

set(SEMANTIC_SRC
    src/semantic/semantic_pass.cc
    src/semantic/semantic_first_pass.cc
)

set(SCOPE_SRC
    src/scope/symbol.cc
    src/scope/scope.cc
)

set(DRIVER_SRC
    src/driver/driver.cc
)

set(PRETTY_PRINTER_SRC
    src/pretty_printer/pretty_printer.cc
)

set(CPP_GENERATOR_SRC
    src/cpp_generator/cpp_generator.cc
)

set(LOG_SRC
    src/log/log.cc
    src/log/logger.cc
)

set(SOURCES
    ${STRING_POOL_SRC}
    ${TYPE_POOL_SRC}
    ${TOKEN_SRC}
    ${AST_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${SCOPE_SRC}
    ${SEMANTIC_SRC}
    ${LOG_SRC}
    ${DRIVER_SRC}
    ${PRETTY_PRINTER_SRC}
    ${CPP_GENERATOR_SRC}
    src/main.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})
