find_package(doctest REQUIRED)
include(CTest)

add_executable(LoggerTest LoggerTest.cpp)
target_include_directories(LoggerTest PRIVATE ${CMAKE_SOURCE_DIR}/include)
# target_link_libraries(LoggerTest PRIVATE ${PROJECT_NAME} doctest::doctest)
add_test(NAME LoggerTest COMMAND LoggerTest)

add_executable(integrationTest integrationTest.cpp)
target_include_directories(integrationTest PRIVATE ${CMAKE_SOURCE_DIR}/include)
# target_link_libraries(integrationTest PRIVATE ${PROJECT_NAME} doctest::doctest)
add_test(NAME integrationTest COMMAND integrationTest)
