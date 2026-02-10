find_package(raylib REQUIRED)

# Retrouver bibliothèque et en-têtes SmartPianoEngine
find_path(ENGINE_INCLUDE_DIR NAMES Message.hpp HINTS "${ENGINE_PATH}/include" REQUIRED)
find_library(ENGINE_LIBRARY NAMES enginecomm HINTS "${ENGINE_PATH}/lib" REQUIRED)

add_executable(main main.cpp)

target_include_directories(
  main
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PUBLIC ${ENGINE_INCLUDE_DIR}
)

target_link_libraries(
  main
  ${ENGINE_LIBRARY}
  raylib
)
if(UNIX AND NOT APPLE)
  target_link_libraries(main m pthread dl rt X11)
endif()
