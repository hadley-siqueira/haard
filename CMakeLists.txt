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

set(SCANNER_SRC
    src/haard/scanner/scanner.cpp
)

set(CONTEXT_SRC
    src/haard/context/context.cpp
)

set(SOURCES
    src/main.cpp
    ${TOKEN_SRC}
    ${SOURCE_FILE_SRC}
    ${SCANNER_SRC}
    ${CONTEXT_SRC}
)

add_executable(${PROJECT_NAME} ${SOURCES})
