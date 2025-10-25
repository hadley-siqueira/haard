cmake_minimum_required(VERSION 2.8)
project(hdc)

include_directories(src/haard)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

set(TOKEN_SRC
    src/haard/token/token.cc
)

set(AST_SRC
    src/haard/ast/ast.cc
)

set(SOURCES
    src/haard/main.cc
    ${TOKEN_SRC}
    ${AST_SRC}
)

add_executable(${PROJECT_NAME} ${SOURCES})
