cmake_minimum_required(VERSION 3.20)
project(
  ui # Smart Piano User Interface
  VERSION 0.0.0
  LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

add_compile_options(
  -Wall -Wextra -Wpedantic -Wshadow
  # -Wconversion -Werror
)

option(COVERAGE "Mesure couverture de code" ON)
if(COVERAGE)
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
  add_link_options(-fprofile-instr-generate)
endif()
set(PROFRAW_MAIN "main.profraw")
set(PROFRAW_TESTS "tests-%m.profraw")
set(PROFDATA_FILE "coverage.profdata")

add_subdirectory(src)
enable_testing()
add_subdirectory(test)

# Exécute les exécutables pour générer données de profilage (main.profraw…)
add_custom_target(
  run
  COMMAND LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/${PROFRAW_MAIN} ./src/main
          --timeout 1000 # S’arrêter tout seul après 1s
  COMMENT "Exécution du main pour générer ${PROFRAW_MAIN}")
add_custom_target(
  tests
  COMMAND rm -f ${CMAKE_BINARY_DIR}/tests-*.profraw
  COMMAND LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/${PROFRAW_TESTS} ctest
  COMMENT "Exécution des tests pour générer ${PROFRAW_TESTS}")
# Fusionne les données de profilage dans coverage.profdata
add_custom_target(
  merge_coverage_data
  COMMAND
    bash -c
    "llvm-profdata merge -sparse -output=${CMAKE_BINARY_DIR}/${PROFDATA_FILE} ${CMAKE_BINARY_DIR}/${PROFRAW_MAIN} ${CMAKE_BINARY_DIR}/tests-*.profraw"
  DEPENDS tests run
  COMMENT "Fusion données couverture: ${PROFDATA_FILE}")
# Génération du rapport de couverture de code HTML
add_custom_target(
  coverage
  COMMAND
    llvm-cov show ./src/main -instr-profile=${CMAKE_BINARY_DIR}/${PROFDATA_FILE}
    -format=html -output-dir=${CMAKE_BINARY_DIR}/coverage
  DEPENDS merge_coverage_data
  COMMENT "Rapport de couverture HTML généré: ${CMAKE_BINARY_DIR}/coverage")
add_dependencies(run main)
add_dependencies(tests LoggerTest)
add_dependencies(tests MessageTest)
add_dependencies(tests UdsTransportTest)
add_dependencies(tests integrationTest)
add_dependencies(coverage merge_coverage_data)
