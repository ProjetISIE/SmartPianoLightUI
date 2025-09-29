cmake_minimum_required(VERSION 3.30)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MAIN SmartPianoUI)
project(${MAIN} LANGUAGES CXX VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(RPI "Build for Raspberry Pi" OFF) # cmake -DRP=ON to target Raspberry Pi

if(RPI)
    message(STATUS "Raspberry Pi cross-compilation")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/raspi.cmake"
        CACHE FILEPATH "path to Raspberry Pi toolchain" FORCE)
endif()

add_executable(${MAIN} main.cpp)

# Strict compiler warnings
target_compile_options(${MAIN} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Werror
)

