cmake_minimum_required (VERSION 3.8)

project ("TestTaskIniti")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 -Wall -Wextra -Wpedantic -Werror ")

add_custom_command(
    OUTPUT raw
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/raw.bin ${CMAKE_CURRENT_BINARY_DIR}/raw.bin
)

add_subdirectory(serializator)

# Example
add_executable(decoder main.cpp raw)

target_link_libraries(decoder Serializer)

# Собираем тесты
add_subdirectory(tests)