cmake_minimum_required(VERSION 2.8)
project(hdc)

include_directories(src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g")

set(SOURCE_FILE_SRC
    src/haard/source_file/source_file.cpp
)

set(TOKEN_SRC
    src/haard/token/token.cpp
)

set(TOKENS_SRC
    src/haard/tokens/tokens.cpp
)

set(AST_SRC
    src/haard/ast/ast_node.cpp
    src/haard/ast/ast.cpp
)

set(SCANNER_SRC
    src/haard/scanner/scanner.cpp
)

set(PARSER_SRC
    src/haard/parser/parser.cpp
)

set(CONTEXT_SRC
    src/haard/context/context.cpp
)


set(SOURCES
    src/main.cpp
    ${TOKEN_SRC}
    ${TOKENS_SRC}
    ${AST_SRC}
    ${SOURCE_FILE_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${CONTEXT_SRC}
)

add_executable(${PROJECT_NAME} ${SOURCES})
