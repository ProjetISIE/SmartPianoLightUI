cmake_minimum_required(VERSION 3.20)
project(
  ui # Smart Piano User Interface
  VERSION 0.0.0
  LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# find_package(raylib REQUIRED)
find_package(doctest REQUIRED)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    # -Wconversion
    # -Werror
)

add_subdirectory(src)
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest) # TESTS
# add_subdirectory(test)
