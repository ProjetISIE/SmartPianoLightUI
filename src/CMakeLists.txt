find_package(raylib REQUIRED)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)

# Retrouver bibliothèque et en-têtes SmartPianoEngine
find_path(
  ENGINE_INCLUDE_DIR
  NAMES Message.hpp
  HINTS "${ENGINE_PATH}/include" REQUIRED
  NO_CMAKE_FIND_ROOT_PATH)
find_library(
  ENGINE_LIBRARY
  NAMES enginecomm
  HINTS "${ENGINE_PATH}/lib" REQUIRED
  NO_CMAKE_FIND_ROOT_PATH)

add_executable(main main.cpp Communication.cpp MusicUtils.cpp UI.cpp
                    AppController.cpp)

target_include_directories(
  main
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PUBLIC ${ENGINE_INCLUDE_DIR}
  PUBLIC /usr/aarch64-linux-gnu/include)

# Chemin du binaire moteur passé par ui.nix via -DENGINE_PATH=…
if(DEFINED ENGINE_PATH)
  target_compile_definitions(
    main PRIVATE ENGINE_BIN_PATH="${ENGINE_PATH}/bin/engine")
endif()

target_link_libraries(main ${ENGINE_LIBRARY} raylib PkgConfig::GLFW)
if(UNIX AND NOT APPLE)
  target_link_libraries(main m pthread dl rt X11)
endif()
