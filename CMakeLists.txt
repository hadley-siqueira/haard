cmake_minimum_required(VERSION 3.0)
project(hdc)

include_directories(src)

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g")

set(STRING_POOL_SRC
    src/haard/string_pool/string_pool.cc
)

set(UTILS_SRC
    src/haard/utils/utils.cc
)

set(LOG_SRC
    src/haard/log/log.cc
    src/haard/log/logger.cc
)

set(TOKEN_SRC
    src/haard/token/token.cc
)

set(SCANNER_SRC
    src/haard/scanner/scanner.cc
)

set(AST_SRC
    src/haard/ast/ast.cc
    src/haard/ast/import.cc
    src/haard/ast/module.cc
    src/haard/ast/variable.cc
    src/haard/ast/type.cc
    src/haard/ast/expression.cc
)

set(PARSER_SRC
    src/haard/parser/parser.cc
)

set(SOURCES
    ${STRING_POOL_SRC}
    ${UTILS_SRC}
    ${LOG_SRC}
    ${TOKEN_SRC}
    ${SCANNER_SRC}
    ${AST_SRC}
    ${PARSER_SRC}
    src/main.cc
)

add_executable(${PROJECT_NAME} ${SOURCES})
