cmake_minimum_required(VERSION 3.10)
project(chess-engine)

set(CMAKE_CXX_STANDARD 17)

# Include header files
include_directories(include)

# Add source files
set(SOURCES
    src/board.cpp
    src/attack_tables.cpp
    src/magic_bitboards.cpp
    # Add other source files here as you create them
)

add_executable(chess ${SOURCES} src/main.cpp)

add_executable(perft ${SOURCES} tests/perft.cpp)