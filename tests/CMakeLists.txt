include(../cmake/GoogleTest.cmake)
include(../cmake/Nlohmann.cmake)

enable_testing()
include(GoogleTest)

# include_directories(${CMAKE_SOURCE_DIR}/lib/include)
set(CMAKE_CXX_STANDARD 17)

add_custom_command(
    OUTPUT raw_tests
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/raw.bin ${CMAKE_CURRENT_BINARY_DIR}/raw.bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/raw.json ${CMAKE_CURRENT_BINARY_DIR}/raw.json
)


add_executable(test_foo raw_tests test_main.cpp)

target_link_libraries(test_foo PRIVATE Serializer GTest::gtest_main GTest::gtest GTest::gtest_main nlohmann_json::nlohmann_json)
target_include_directories(
        Serializer ${warning_guard}
        PUBLIC
        "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/serializator>"
)

gtest_discover_tests(test_foo)