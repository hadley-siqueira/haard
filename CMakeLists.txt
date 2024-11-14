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
    src/haard/ast/expressions/operators/binary/generics_application.cc
    src/haard/ast/expressions/operators/binary/scope.cc
    src/haard/ast/expressions/identifier.cc
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

set(SEMANTIC_SRC
    src/haard/semantic/semantic_analyzer.cc
)

set(SCOPE_SRC
    src/haard/scope/scope.cc
)

set(SYMBOL_SRC
    src/haard/symbol/symbol.cc
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
    ${SYMBOL_SRC}
    ${SCOPE_SRC}
    ${SEMANTIC_SRC}
    ${LOG_SRC}
    ${DRIVER_SRC}
    ${PRETTY_PRINTER_SRC}
    src/main.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})
