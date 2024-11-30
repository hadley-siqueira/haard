cmake_minimum_required(VERSION 3.0)
project(hdc)

include_directories(src/include)

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g")

set(UTILS_SRC
    src/haard/utils/utils.cc
)

set(TOKEN_SRC
    src/haard/token/token.cc
)

set(AST_SRC
    src/haard/ast/ast.cc
    src/haard/ast/modules.cc
    src/haard/ast/module.cc
    src/haard/ast/import.cc
    src/haard/ast/function.cc
    src/haard/ast/variable.cc
    src/haard/ast/generics.cc
    src/haard/ast/types/type.cc
    src/haard/ast/types/primitive_type.cc
    src/haard/ast/types/pointer_type.cc
    src/haard/ast/types/reference_type.cc
    src/haard/ast/types/list_type.cc
    src/haard/ast/types/array_type.cc
    src/haard/ast/types/tuple_type.cc
    src/haard/ast/types/function_type.cc
    src/haard/ast/types/named_type.cc

    src/haard/ast/statements/statement.cc
    src/haard/ast/statements/statements.cc
    src/haard/ast/statements/while_statement.cc
    src/haard/ast/statements/return_statement.cc
    src/haard/ast/statements/expression_statement.cc

    src/haard/ast/expressions/expression.cc

    src/haard/ast/expressions/operators/binary/binary_operator.cc
    src/haard/ast/expressions/operators/binary/assignment.cc
    src/haard/ast/expressions/operators/binary/bitwise_and_assignment.cc
    src/haard/ast/expressions/operators/binary/bitwise_or_assignment.cc
    src/haard/ast/expressions/operators/binary/bitwise_xor_assignment.cc
    src/haard/ast/expressions/operators/binary/bitwise_not_assignment.cc
    src/haard/ast/expressions/operators/binary/division_assignment.cc
    src/haard/ast/expressions/operators/binary/integer_division_assignment.cc
    src/haard/ast/expressions/operators/binary/minus_assignment.cc
    src/haard/ast/expressions/operators/binary/modulo_assignment.cc
    src/haard/ast/expressions/operators/binary/plus_assignment.cc
    src/haard/ast/expressions/operators/binary/times_assignment.cc
    src/haard/ast/expressions/operators/binary/shift_left_logical_assignment.cc
    src/haard/ast/expressions/operators/binary/shift_right_logical_assignment.cc
    src/haard/ast/expressions/operators/binary/shift_right_arithmetic_assignment.cc

    src/haard/ast/expressions/operators/binary/logical_and.cc
    src/haard/ast/expressions/operators/binary/logical_or.cc
    src/haard/ast/expressions/operators/binary/equal.cc
    src/haard/ast/expressions/operators/binary/not_equal.cc
    src/haard/ast/expressions/operators/binary/less_than.cc
    src/haard/ast/expressions/operators/binary/greater_than.cc
    src/haard/ast/expressions/operators/binary/less_than_or_equal.cc
    src/haard/ast/expressions/operators/binary/greater_than_or_equal.cc
    src/haard/ast/expressions/operators/binary/in.cc
    src/haard/ast/expressions/operators/binary/not_in.cc

    src/haard/ast/expressions/operators/binary/inclusive_range.cc
    src/haard/ast/expressions/operators/binary/exclusive_range.cc
    src/haard/ast/expressions/operators/binary/plus.cc
    src/haard/ast/expressions/operators/binary/minus.cc
    src/haard/ast/expressions/operators/binary/times.cc
    src/haard/ast/expressions/operators/binary/division.cc
    src/haard/ast/expressions/operators/binary/modulo.cc
    src/haard/ast/expressions/operators/binary/integer_division.cc
    src/haard/ast/expressions/operators/binary/power.cc
    src/haard/ast/expressions/operators/binary/shift_left_logical.cc
    src/haard/ast/expressions/operators/binary/shift_right_logical.cc
    src/haard/ast/expressions/operators/binary/shift_right_arithmetic.cc
    src/haard/ast/expressions/operators/binary/bitwise_and.cc
    src/haard/ast/expressions/operators/binary/bitwise_or.cc
    src/haard/ast/expressions/operators/binary/bitwise_xor.cc
    src/haard/ast/expressions/operators/binary/index.cc
    src/haard/ast/expressions/operators/binary/dot.cc
    src/haard/ast/expressions/operators/binary/arrow.cc

    src/haard/ast/expressions/operators/binary/generics_application.cc
    src/haard/ast/expressions/operators/binary/scope.cc

    src/haard/ast/expressions/operators/unary/unary_operator.cc
    src/haard/ast/expressions/operators/unary/unary_plus.cc
    src/haard/ast/expressions/operators/unary/unary_minus.cc
    src/haard/ast/expressions/operators/unary/address_of.cc
    src/haard/ast/expressions/operators/unary/dereference.cc
    src/haard/ast/expressions/operators/unary/bitwise_not.cc
    src/haard/ast/expressions/operators/unary/logical_not.cc
    src/haard/ast/expressions/operators/unary/pre_increment.cc
    src/haard/ast/expressions/operators/unary/pre_decrement.cc
    src/haard/ast/expressions/operators/unary/pos_increment.cc
    src/haard/ast/expressions/operators/unary/pos_decrement.cc

    src/haard/ast/expressions/identifier.cc
    src/haard/ast/expressions/this.cc
    src/haard/ast/expressions/null.cc

    src/haard/ast/expressions/literals/literal.cc
    src/haard/ast/expressions/literals/boolean_literal.cc
    src/haard/ast/expressions/literals/char_literal.cc
    src/haard/ast/expressions/literals/integer_literal.cc
    src/haard/ast/expressions/literals/float_literal.cc
    src/haard/ast/expressions/literals/string_literal.cc
    src/haard/ast/expressions/literals/symbol_literal.cc
)

set(IR_SRC
    src/ir/ir.cc
    src/ir/ir_value.cc
    src/ir/ir_binop.cc
)

set(SCANNER_SRC
    src/haard/scanner/scanner.cc
)

set(PARSER_SRC
    src/haard/parser/parser.cc
)

set(DRIVER_SRC
    src/haard/driver/driver.cc
)

set(PRETTY_PRINTER_SRC
    src/haard/pretty_printer/pretty_printer.cc
)

set(LOG_SRC
    src/haard/log/log.cc
    src/haard/log/logger.cc
    src/haard/log/logs.cc
)

set(SOURCES
    ${UTILS_SRC}
    ${TOKEN_SRC}
    ${AST_SRC}
    ${IR_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${LOG_SRC}
    ${DRIVER_SRC}
    ${PRETTY_PRINTER_SRC}
    src/main.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})
