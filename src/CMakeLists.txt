find_package(imgui CONFIG REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui::imgui # The target from imgui-config.cmake
    glfw         # GLFW target
    OpenGL::GL   # OpenGL target
)
