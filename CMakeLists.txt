cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Useful for LSPs and tools

set(MAIN SmartPianoUI)
project(${MAIN} LANGUAGES CXX VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${MAIN} main.cpp)

target_compile_options(${MAIN} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Werror
)
