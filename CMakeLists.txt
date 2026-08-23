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
    ${SOURCE_FILE_SRC}
    ${SCANNER_SRC}
    ${PARSER_SRC}
    ${CONTEXT_SRC}
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
    COMMAND ${CMAKE_SOURCE_DIR}/tests/pretty_printer/run.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running the tests"
    VERBATIM
)
