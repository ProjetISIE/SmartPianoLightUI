find_package(raylib REQUIRED)
find_package(engine REQUIRED)

add_executable(main main.cpp)

target_include_directories(main PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(
  main
  engine
  raylib
)
if(UNIX AND NOT APPLE)
  target_link_libraries(main m pthread dl rt X11)
endif()
