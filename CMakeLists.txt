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
    src/haard/ast/ast_builder.cpp
)

set(AST_QUERY_SRC
    src/haard/ast_query/ast_query.cpp
)

set(STRING_TABLE_SRC
    src/haard/string_table/string_table.cpp
)

set(TYPE_TABLE_SRC
    src/haard/type_table/type_table.cpp
    src/haard/type_table/type_builder.cpp
    src/haard/type_table/type_collector.cpp
    src/haard/type_table/expression_typer.cpp
)

set(SYMBOL_TABLE_SRC
    src/haard/symbol_table/symbol_table.cpp
    src/haard/symbol_table/symbol_collector.cpp
)

set(MODULE_FINDER_SRC
    src/haard/module_finder/module_finder.cpp
)

set(NAME_RESOLVER_SRC
    src/haard/name_resolver/name_resolver.cpp
    src/haard/name_resolver/use_resolver.cpp
    src/haard/name_resolver/overload_resolver.cpp
)

set(COMPILATION_SRC
    src/haard/compilation/compilation.cpp
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

set(MODULE_SRC
    src/haard/module/module.cpp
)

set(LOG_SRC
    src/haard/log/log.cpp
)

set(DRIVER_SRC
    src/haard/driver/driver.cpp
)


set(SOURCES
    src/main.cpp
    ${TOKEN_SRC}
    ${TOKEN_STREAM_SRC}
    ${AST_SRC}
    ${AST_QUERY_SRC}
    ${STRING_TABLE_SRC}
    ${SYMBOL_TABLE_SRC}
    ${TYPE_TABLE_SRC}
    ${MODULE_FINDER_SRC}
    ${COMPILATION_SRC}
    ${NAME_RESOLVER_SRC}
    ${SOURCE_FILE_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${MODULE_SRC}
    ${LOG_SRC}
    ${DRIVER_SRC}
    ${PRETTY_SRC}
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Code formatting with clang-format.
# Use -DCLANG_FORMAT_EXECUTABLE=/path/to/clang-format to pick another binary.
find_program(CLANG_FORMAT_EXECUTABLE
    NAMES clang-format clang-format-18 clang-format-17
)

file(GLOB_RECURSE FORMAT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

if(CLANG_FORMAT_EXECUTABLE)
    # make format: rewrites the sources in the .clang-format style
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i --style=file ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting the sources with clang-format"
        VERBATIM
    )

    # make format-check: changes nothing, fails if anything is out of style
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run -Werror --style=file ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking the formatting of the sources"
        VERBATIM
    )
else()
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found. Install it with: sudo apt install clang-format"
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM
    )

    add_custom_target(format-check DEPENDS format)
endif()

# make check: runs every test suite
add_custom_target(check
    COMMAND ${CMAKE_SOURCE_DIR}/tests/scanner/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/parser/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/ast_query/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/string_table/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/symbol_table/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/type_table/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/module_finder/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/compilation/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/name_resolver/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/use_resolver/run.sh
    COMMAND ${CMAKE_SOURCE_DIR}/tests/pretty_printer/run.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running the tests"
    VERBATIM
)
