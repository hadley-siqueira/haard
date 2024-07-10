cmake_minimum_required(VERSION 3.0)
project(hdc)

include_directories(src/include)

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g")

set(UTILS_SRC
    src/utils/utils.cc
)

set(STRING_POOL_SRC
    src/string_pool/string_pool.cc
)

set(TOKEN_SRC
    src/haard/token/token.cc
)

set(AST_SRC
    src/haard/ast/ast.cc
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
    src/scanner/scanner.cc
)

set(PARSER_SRC
    src/parser/parser.cc
)

set(SEMANTIC_SRC
    src/semantic/semantic_analyser.cc
    src/semantic/semantic_pass.cc
    src/semantic/semantic_define_pass.cc
    src/semantic/modules_semantic_analyser.cc
    src/semantic/module_semantic_analyser.cc
    src/semantic/module_named_type_definer.cc
    src/semantic/module_function_definer.cc
    src/semantic/type_linker.cc
    src/semantic/semantic_context.cc
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

set(CPP_GENERATOR_SRC
#    src/cpp_generator/cpp_generator.cc
#    src/cpp_generator/modules_cpp_generator.cc
#    src/cpp_generator/module_cpp_generator.cc
#    src/cpp_generator/class_cpp_generator.cc
#    src/cpp_generator/function_cpp_generator.cc
#    src/cpp_generator/statement_cpp_generator.cc
#    src/cpp_generator/expression_cpp_generator.cc
)

set(LOG_SRC
    src/log/log.cc
    src/log/logger.cc
    src/log/logs.cc
)

set(SOURCES
    ${UTILS_SRC}
    ${STRING_POOL_SRC}
    ${TYPE_POOL_SRC}
    ${JSON_SRC}
    ${TOKEN_SRC}
    ${AST_SRC}
    ${IR_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${SCOPE_SRC}
    #${SEMANTIC_SRC}
    ${LOG_SRC}
    ${DRIVER_SRC}
    ${PRETTY_PRINTER_SRC}
    #${CPP_GENERATOR_SRC}
    src/main.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})
