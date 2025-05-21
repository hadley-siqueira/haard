cmake_minimum_required(VERSION 3.0)
project(hdc)

include_directories(src/include)

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g")

set(UTILS_SRC
    src/haard/utils/utils.cc
)

set(STRING_POOL_SRC
    src/haard/string_pool/string_pool.cc
)

set(TOKEN_SRC
    src/haard/token/token.cc
)

set(AST_SRC
    src/haard/ast/ast.cc
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
    ${STRING_POOL_SRC}
    ${TOKEN_SRC}
    ${AST_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${MODULES_MANAGER_SRC}
    ${SYMBOL_SRC}
    ${SCOPE_SRC}
    ${SEMANTIC_SRC}
    ${LOG_SRC}
    ${DRIVER_SRC}
    ${PRETTY_PRINTER_SRC}
    src/main.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})
