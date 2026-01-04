# add_library(
#   ${PROJECT_NAME}Lib STATIC
# )
# target_include_directories(${PROJECT_NAME}Lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
# target_link_libraries(
#   ${PROJECT_NAME}Lib
#   PUBLIC
# )

add_executable(${PROJECT_NAME} main.cpp)
# target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}Lib)
