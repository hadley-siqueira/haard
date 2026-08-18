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

set(TOKEN_STREAM_SRC
    src/haard/token_stream/token_stream.cpp
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

set(PRETTY_SRC
    src/haard/pretty_printer/pretty_printer.cpp
)

set(CONTEXT_SRC
    src/haard/context/context.cpp
)


set(SOURCES
    src/main.cpp
    ${TOKEN_SRC}
    ${TOKEN_STREAM_SRC}
    ${AST_SRC}
    ${SOURCE_FILE_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${CONTEXT_SRC}
    ${PRETTY_SRC}
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Formatacao de codigo com clang-format.
# Use -DCLANG_FORMAT_EXECUTABLE=/caminho/para/clang-format para escolher outro binario.
find_program(CLANG_FORMAT_EXECUTABLE
    NAMES clang-format clang-format-18 clang-format-17
)

file(GLOB_RECURSE FORMAT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

if(CLANG_FORMAT_EXECUTABLE)
    # make format: reescreve os fontes no estilo de .clang-format
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i --style=file ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatando os fontes com clang-format"
        VERBATIM
    )

    # make format-check: nao altera nada, falha se algo estiver fora do padrao
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run -Werror --style=file ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Verificando a formatacao dos fontes"
        VERBATIM
    )
else()
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format nao encontrado. Instale com: sudo apt install clang-format"
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM
    )

    add_custom_target(format-check DEPENDS format)
endif()
