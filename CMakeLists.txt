cmake_minimum_required(VERSION 2.8)
project(hdc)

include_directories(src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g")

set(SOURCE_FILE_SRC
    src/haard/source_file/source_file.cpp
)

set(SOURCES
    src/main.cpp
    ${SOURCE_FILE_SRC}
)

add_executable(${PROJECT_NAME} ${SOURCES})
