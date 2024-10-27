cmake_minimum_required(VERSION 3.0)
project(hdc)

include_directories(src/include)

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g")

set(UTILS_SRC
    src/utils/utils.cc
)

set(TOKEN_SRC
    src/haard/token/token.cc
)

set(AST_SRC
    src/haard/ast/ast.cc
    src/haard/ast/module.cc
    src/haard/ast/import.cc
    src/haard/ast/function.cc
    src/haard/ast/variable.cc
    src/haard/ast/type.cc
)

set(DESCRIPTORS_SRC
    src/haard/descriptors/module.cc
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

set(MODULES_MANAGER_SRC
    src/haard/modules_manager/modules_manager.cc
)

set(SCOPE_SRC
    src/scope/symbol_descriptor.cc
    src/scope/symbol.cc
    src/scope/scope.cc
)

set(DRIVER_SRC
    src/driver/driver.cc
)

set(PRETTY_PRINTER_SRC
    src/pretty_printer/pretty_printer.cc
)

set(LOG_SRC
    src/log/log.cc
    src/log/logger.cc
    src/log/logs.cc
)

set(SOURCES
    ${UTILS_SRC}
    ${TOKEN_SRC}
    ${AST_SRC}
    ${IR_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${MODULES_MANAGER_SRC}
    ${SCOPE_SRC}
    ${LOG_SRC}
    ${DRIVER_SRC}
    ${PRETTY_PRINTER_SRC}
    src/main.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})
