cmake_minimum_required(VERSION 2.8)
project(hdc)

include_directories(src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

set(STRING_POOL_SRC
    src/haard/string_pool/string_pool.cc
)

set(LOG_SRC
    src/haard/log/log.cc
    src/haard/log/logger.cc
)

set(TOKEN_SRC
    src/haard/token/token.cc
)

set(AST_SRC
    src/haard/ast/ast.cc
    src/haard/ast/ast_n.cc
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

set(SOURCES
    src/haard/main.cc
    ${STRING_POOL_SRC}
    ${LOG_SRC}
    ${TOKEN_SRC}
    ${AST_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${DRIVER_SRC}
)

add_executable(${PROJECT_NAME} ${SOURCES})
